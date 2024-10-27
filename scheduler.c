#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Headers as needed

typedef enum {false, true} bool;        // Allows boolean types in C

/* Defines a job struct */
typedef struct Process {
    uint32_t A;                         // A: Arrival time of the process
    uint32_t B;                         // B: Upper Bound of CPU burst times of the given random integer list
    uint32_t C;                         // C: Total CPU time required
    uint32_t M;                         // M: Multiplier of CPU burst time
    uint32_t processID;                 // The process ID given upon input read

    uint8_t status;                     // 0 is unstarted, 1 is ready, 2 is running, 3 is blocked, 4 is terminated

    int32_t finishingTime;              // The cycle when the the process finishes (initially -1)
    uint32_t currentCPUTimeRun;         // The amount of time the process has already run (time in running state)
    uint32_t currentIOBlockedTime;      // The amount of time the process has been IO blocked (time in blocked state)
    uint32_t currentWaitingTime;        // The amount of time spent waiting to be run (time in ready state)
    uint32_t remainingTime;   // Remaining CPU time for this process
    uint32_t remainingIOburst;
    uint32_t currentCPUburst;
    uint32_t lastScheduledTime;
    uint32_t totalCPUTime;
    uint32_t turnaroundTime;

    uint32_t IOBurst;                   // The amount of time until the process finishes being blocked
    uint32_t CPUBurst;                  // The CPU availability of the process (has to be > 1 to move to running)

    int32_t quantum;                    // Used for schedulers that utilise pre-emption

    bool isFirstTimeRunning;            // Used to check when to calculate the CPU burst when it hits running mode

    struct Process* nextInBlockedList;  // A pointer to the next process available in the blocked list
    struct Process* nextInReadyQueue;   // A pointer to the next process available in the ready queue
    struct Process* nextInReadySuspendedQueue; // A pointer to the next process available in the ready suspended queue
} _process;


uint32_t CURRENT_CYCLE = 0;             // The current cycle that each process is on
uint32_t TOTAL_CREATED_PROCESSES = 0;   // The total number of processes constructed
uint32_t TOTAL_STARTED_PROCESSES = 0;   // The total number of processes that have started being simulated
uint32_t TOTAL_FINISHED_PROCESSES = 0;  // The total number of processes that have finished running
uint32_t TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = 0; // The total cycles in the blocked state

const char* RANDOM_NUMBER_FILE_NAME= "random-numbers";
const uint32_t SEED_VALUE = 200;  // Seed value for reading from file

// Additional variables as needed


/**
 * Reads a random non-negative integer X from a file with a given line named random-numbers (in the current directory)
 */
uint32_t getRandNumFromFile(uint32_t line, FILE* random_num_file_ptr){
    uint32_t end, loop;
    char str[512];

    rewind(random_num_file_ptr); // reset to be beginning
    for(end = loop = 0;loop<line;++loop){
        if(0==fgets(str, sizeof(str), random_num_file_ptr)){ //include '\n'
            end = 1;  //can't input (EOF)
            break;
        }
    }
    if(!end) {
        return (uint32_t) atoi(str);
    }

    // fail-safe return
    return (uint32_t) 1804289383;
}



/**
 * Reads a random non-negative integer X from a file named random-numbers.
 * Returns the CPU Burst: : 1 + (random-number-from-file % upper_bound)
 */
uint32_t randomOS(uint32_t upper_bound, uint32_t process_indx, FILE* random_num_file_ptr)
{
    char str[20];
    
    uint32_t unsigned_rand_int = (uint32_t) getRandNumFromFile(SEED_VALUE+process_indx, random_num_file_ptr);
    uint32_t returnValue = 1 + (unsigned_rand_int % upper_bound);

    return returnValue;
} 


/********************* SOME PRINTING HELPERS *********************/


/**
 * Prints to standard output the original input
 * process_list is the original processes inputted (in array form)
 */
void printStart(_process process_list[])
{
    printf("The original input was: %i", TOTAL_CREATED_PROCESSES);

    uint32_t i = 0;
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        printf(" ( %i %i %i %i)", process_list[i].A, process_list[i].B,
               process_list[i].C, process_list[i].M);
    }
    printf("\n");
} 

/**
 * Prints to standard output the final output
 * finished_process_list is the terminated processes (in array form) in the order they each finished in.
 */
void printFinal(_process finished_process_list[])
{
    printf("The (sorted) input is: %i", TOTAL_CREATED_PROCESSES);

    uint32_t i = 0;
    for (; i < TOTAL_FINISHED_PROCESSES; ++i)
    {
        printf(" ( %i %i %i %i)", finished_process_list[i].A, finished_process_list[i].B,
               finished_process_list[i].C, finished_process_list[i].M);
    }
    printf("\n");
} // End of the print final function

/**
 * Prints out specifics for each process.
 * @param process_list The original processes inputted, in array form
 */
void printProcessSpecifics(_process process_list[])
{
    uint32_t i = 0;
    printf("\n");
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        printf("Process %i:\n", process_list[i].processID);
        printf("\t(A,B,C,M) = (%i,%i,%i,%i)\n", process_list[i].A, process_list[i].B,
               process_list[i].C, process_list[i].M);
        printf("\tFinishing time: %i\n", process_list[i].finishingTime);
        printf("\tTurnaround time: %i\n", process_list[i].finishingTime - process_list[i].A);
        printf("\tI/O time: %i\n", process_list[i].currentIOBlockedTime);
        printf("\tWaiting time: %i\n", process_list[i].currentWaitingTime);
        printf("\n");
    }
} // End of the print process specifics function

/**
 * Prints out the summary data
 * process_list The original processes inputted, in array form
 */
void printSummaryData(_process process_list[])
{
    uint32_t i = 0;
    double total_amount_of_time_utilizing_cpu = 0.0;
    double total_amount_of_time_io_blocked = 0.0;
    double total_amount_of_time_spent_waiting = 0.0;
    double total_turnaround_time = 0.0;
    uint32_t final_finishing_time = CURRENT_CYCLE - 1;
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        total_amount_of_time_utilizing_cpu += process_list[i].currentCPUTimeRun;
        total_amount_of_time_io_blocked += process_list[i].currentIOBlockedTime;
        total_amount_of_time_spent_waiting += process_list[i].currentWaitingTime;
        total_turnaround_time += (process_list[i].finishingTime - process_list[i].A);
    }

    // Calculates the CPU utilisation
    double cpu_util = total_amount_of_time_utilizing_cpu / final_finishing_time;

    // Calculates the IO utilisation
    double io_util = (double) TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED / final_finishing_time;

    // Calculates the throughput (Number of processes over the final finishing time times 100)
    double throughput =  100 * ((double) TOTAL_CREATED_PROCESSES/ final_finishing_time);

    // Calculates the average turnaround time
    double avg_turnaround_time = total_turnaround_time / TOTAL_CREATED_PROCESSES;

    // Calculates the average waiting time
    double avg_waiting_time = total_amount_of_time_spent_waiting / TOTAL_CREATED_PROCESSES;

    printf("Summary Data:\n");
    printf("\tFinishing time: %i\n", CURRENT_CYCLE - 1);
    printf("\tCPU Utilisation: %6f\n", cpu_util);
    printf("\tI/O Utilisation: %6f\n", io_util);
    printf("\tThroughput: %6f processes per hundred cycles\n", throughput);
    printf("\tAverage turnaround time: %6f\n", avg_turnaround_time);
    printf("\tAverage waiting time: %6f\n", avg_waiting_time);
} // End of the print summary data function

/* compareProcesses()
 *  Functionality: Function to compare processes based on arrival time
 *  Arguments: Two comparabled processes
 *  Return: int
 */
int compareProcesses(const void *a, const void *b) {
    _process *processA = (_process *)a;
    _process *processB = (_process *)b;
    return (processA->A - processB->A);
}


// Update waiting time for all ready processes
void updateWaitingTime(_process process_list[], uint32_t burst) {
    for (uint32_t i = 0; i < TOTAL_CREATED_PROCESSES; ++i) {
        if (process_list[i].status == 1 && process_list[i].A <= CURRENT_CYCLE) {
            process_list[i].currentWaitingTime += burst;
        }
    }
}

// Handle I/O blocking and update the status of blocked processes
void updateIOBlockedProcesses(_process process_list[]) {
    for (uint32_t i = 0; i < TOTAL_CREATED_PROCESSES; ++i) {
        if (process_list[i].status == 3 && process_list[i].remainingIOburst > 0) {
            process_list[i].remainingIOburst--;
            process_list[i].currentIOBlockedTime++;

            if (process_list[i].remainingIOburst == 0) {
                process_list[i].status = 1;  // Ready
            }
        }
    }
}

/* executeFCFS()
 *  Functionality: Execute the FCFS scheduling policy
 *  Arguments: Array of process to run and their number
 *  Return: void
 */
void executeFCFS(_process process_list[], uint32_t num_processes) {
    uint32_t current_time = 0;

    // Sort processes by arrival time
    qsort(process_list, num_processes, sizeof(_process), compareProcesses);

    for (uint32_t i = 0; i < num_processes; ++i) {
        _process *current_process = &process_list[i];

        // If the process arrives after the current time, fast forward time
        if (current_process->A > current_time) {
            current_time = current_process->A;
        }

        // Calculate waiting time before the process starts
        current_process->currentWaitingTime += (current_time - current_process->A);

        // Simulate CPU burst
        uint32_t cpu_burst_time = current_process->C; // Total CPU burst
        current_process->currentCPUTimeRun += cpu_burst_time; // Add to CPU time run
        current_time += cpu_burst_time; // Advance time after CPU burst

        // Check if the process needs an I/O burst after this CPU burst
        if (current_process->M > 0) {
            // Process enters blocked state for I/O
            current_process->status = 3; // Blocked (I/O)
            uint32_t io_burst_time = current_process->M; // I/O burst time (as specified for the process)

            current_process->currentIOBlockedTime += io_burst_time; // Add I/O blocked time
            current_time += io_burst_time; // Advance time for I/O burst

            // After I/O burst, the process will re-enter ready state (but in FCFS, it runs directly)
            current_process->status = 2; // Running
        }

        // Once the process finishes, we record its finishing time
        current_process->status = 4; // Terminated
        current_process->finishingTime = current_time; // Set finishing time

        // Update the total finished processes
        TOTAL_FINISHED_PROCESSES++;
    }

    // Set the current cycle to the last processed time
    CURRENT_CYCLE = current_time;
}




/* executeRR()
 *  Functionality: Simulates the Round Robin scheduling policy
 *  Arguments: processes scheduled, the number and desired quantum for each process
 *  Return: void
 */
void executeRR(_process process_list[], uint32_t num_processes, uint32_t quantum) {
    uint32_t current_time = 0;
    uint32_t process_remaining = num_processes;
    _process *ready_queue[num_processes]; // Array to act as the ready queue
    uint32_t head = 0, tail = 0; // Pointers for the queue

    // Sort processes by arrival time
    qsort(process_list, num_processes, sizeof(_process), compareProcesses);

    while (process_remaining > 0) {
        // Check for new arrivals and add them to the ready queue
        for (uint32_t i = 0; i < num_processes; ++i) {
            if (process_list[i].A <= current_time && process_list[i].status == 0) {
                ready_queue[tail++] = &process_list[i]; // Add to the ready queue
                process_list[i].status = 1; // Mark as ready
            }
        }

        // If there are no processes in the ready queue, fast forward time
        if (head == tail) {
            current_time++;
            continue; // No process to run
        }

        // Get the next process in the ready queue
        _process *current_process = ready_queue[head++];
        head %= num_processes; // Wrap around the head pointer for circular queue

        // Check if the process is blocked due to I/O
        if (current_process->status == 3) {
            if (current_process->IOBurst > 0) {
                current_process->IOBurst--; // Reduce I/O burst time
                current_process->currentIOBlockedTime++; // Track I/O time
                current_time++; // Advance time
                continue;
            } else {
                // I/O burst complete, reinsert into the ready queue
                current_process->status = 1; // Mark as ready
                ready_queue[tail++] = current_process;
                tail %= num_processes;
                continue;
            }
        }

        // Track waiting time: how long the process was in the ready queue
        if (current_process->lastScheduledTime > 0) {
            current_process->currentWaitingTime += (current_time - current_process->lastScheduledTime);
        }
        current_process->lastScheduledTime = current_time;

        // Execute for the quantum or remaining CPU burst, whichever is smaller
        uint32_t time_slice = (current_process->CPUBurst > quantum) ? quantum : current_process->CPUBurst;

        // Update CPU burst and run time
        current_process->CPUBurst -= time_slice;
        current_process->currentCPUTimeRun += time_slice;
        current_time += time_slice; // Advance time by time slice

        // If the process still has CPU burst remaining, requeue it
        if (current_process->CPUBurst > 0) {
            // After running for quantum, check if it needs I/O burst
            if (current_process->IOBurst > 0) {
                current_process->status = 3; // Blocked (I/O)
            } else {
                ready_queue[tail++] = current_process; // Reinsert into ready queue
                tail %= num_processes;
            }
        } else {
            // Process finishes its current CPU burst, check for I/O or terminate
            if (current_process->IOBurst > 0) {
                current_process->status = 3; // Blocked (I/O)
            } else {
                // Process is done, no more CPU or I/O bursts, terminate it
                current_process->status = 4; // Terminated
                current_process->finishingTime = current_time;
                process_remaining--; // Decrease remaining process count

                // Calculate final turnaround time and waiting time
                current_process->turnaroundTime = current_process->finishingTime - current_process->A;
                current_process->currentWaitingTime = current_process->turnaroundTime - current_process->totalCPUTime;
            }
        }
    }

    CURRENT_CYCLE = current_time; // Set the current cycle to the last time processed
}



void printProcessStates(_process process_list[]) {
    printf("\nCurrent Cycle: %u\n", CURRENT_CYCLE);
    for (uint32_t i = 0; i < TOTAL_CREATED_PROCESSES; ++i) {
        printf("Process %u: Status: %u, Remaining CPU Time: %u\n", process_list[i].processID, process_list[i].status, process_list[i].remainingTime);
    }
    printf("\n");
}

int compareProcesses2(const void *a, const void *b) {
    _process *procA = (_process *)a;
    _process *procB = (_process *)b;

    if (procA->A != procB->A) {
        return procA->A - procB->A; // Sort by arrival time
    }
    return procA->processID - procB->processID; // Sort by PID for tie-breaking
}

/* executeSJF()
 *  Functionality: Simulate shortest job first scheduling policy.
 *  Arguments: List of processes
 *  Return: void
 */
void executeSJF(_process process_list[], uint32_t num_processes) {
    uint32_t current_time = 0;
    uint32_t process_remaining = num_processes;

    // Sort processes by arrival time
    qsort(process_list, num_processes, sizeof(_process), compareProcesses2);

    while (process_remaining > 0) {
        // Find the process with the smallest CPU burst time that is ready
        _process *next_process = NULL;

        for (uint32_t i = 0; i < num_processes; ++i) {
            // Only consider processes that have arrived and are not terminated
            if (process_list[i].A <= current_time && process_list[i].status != 4) {
                // If the process is ready and has the smallest burst time
                if (next_process == NULL || 
                    (process_list[i].C < next_process->C) || 
                    (process_list[i].C == next_process->C && process_list[i].processID < next_process->processID)) {
                    next_process = &process_list[i];
                }
            }
        }

        // If no process is ready, fast-forward time to the next process's arrival
        if (next_process == NULL) {
            // Find the next process arrival time
            for (uint32_t i = 0; i < num_processes; ++i) {
                if (process_list[i].status != 4) { // Not terminated
                    if (next_process == NULL || process_list[i].A < next_process->A) {
                        next_process = &process_list[i];
                    }
                }
            }
            // Fast-forward time to the next process's arrival time
            if (next_process != NULL) {
                current_time = next_process->A;
                continue;
            }
        } else {
            // Update the process status to running
            next_process->status = 2; // Mark as running

            // Simulate the process running
            current_time += next_process->C; // Advance time by CPU burst time
            next_process->finishingTime = current_time; // Set finishing time
            next_process->status = 4; // Mark as terminated
            next_process->CPUBurst = 0; // Process is finished
            process_remaining--; // Decrease remaining process count

            // Calculate turnaround time and waiting time
            next_process->currentWaitingTime = next_process->finishingTime - next_process->A - next_process->C;
        }
    }

    CURRENT_CYCLE = current_time; // Set the current cycle to the last time processed
}



void initializeProcesses(_process process_list[], uint32_t total_num_of_process, FILE* input_file) {
    for (uint32_t i = 0; i < total_num_of_process; ++i) {
        uint32_t A, B, C, M;
        
        // Read each process's details from the input file
        fscanf(input_file, " (%u %u %u %u)", &A, &B, &C, &M);

        // Initialize process attributes
        process_list[i].A = A;  // Arrival time
        process_list[i].B = B;  // CPU burst time
        process_list[i].C = C;  // Total CPU time needed
        process_list[i].M = M;  // Multiplier for I/O burst
        
        process_list[i].processID = i;  // Assign unique process ID
        
        // Initial status
        process_list[i].status = 0;  // Status: 0 = unstarted
        process_list[i].finishingTime = -1;  // Finishing time initially set to -1
        
        // CPU and execution values
        process_list[i].currentCPUTimeRun = 0;  // No CPU time run yet
        process_list[i].remainingTime = process_list[i].C;  // Remaining CPU time is initially total time
        process_list[i].currentCPUburst = 0;  // No CPU burst has been started yet
        process_list[i].CPUBurst = 0;  // Initially set to 0, updated during execution
        
        // I/O values
        process_list[i].currentIOBlockedTime = 0;  // No I/O blocked time yet
        process_list[i].remainingIOburst = 0;  // No remaining I/O burst yet
        process_list[i].IOBurst = 0;  // I/O burst will be calculated during execution
        
        // Waiting and turnaround times
        process_list[i].currentWaitingTime = 0;  // Waiting time starts at 0
        process_list[i].turnaroundTime = 0;  // Turnaround time starts at 0
        
        // Flags for first-time execution
        process_list[i].quantum = 0;  // Quantum, only used for Round Robin, set to 0 for other algorithms
        process_list[i].isFirstTimeRunning = true;  // Set to true, indicating it hasn't been scheduled yet
    }
}



int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* input_file_name = argv[1];
    FILE* input_file = fopen(input_file_name, "r");
    if (!input_file) {
        perror("Failed to open input file");
        return EXIT_FAILURE;
    }

    uint32_t total_num_of_process;  
    fscanf(input_file, "%u", &total_num_of_process);

    _process process_list[total_num_of_process];
    TOTAL_CREATED_PROCESSES = total_num_of_process;

    // Initialize processes for the first scheduling algorithm
    initializeProcesses(process_list, total_num_of_process, input_file);
    fclose(input_file); // Close the input file after first initialization

    // Print the original input
    printStart(process_list);

    // First Come First Serve
    printf("######################### START OF FIRST COME FIRST SERVE #########################\n");
    executeFCFS(process_list, TOTAL_CREATED_PROCESSES);
    printFinal(process_list);
    printProcessSpecifics(process_list);
    printSummaryData(process_list);
    printf("######################### END OF FIRST COME FIRST SERVE #########################\n");

    // Reopen the input file for the Round Robin scheduling
    input_file = fopen(input_file_name, "r");
    if (!input_file) {
        perror("Failed to reopen input file");
        return EXIT_FAILURE;
    }
    initializeProcesses(process_list, total_num_of_process, input_file);
    fclose(input_file);

    uint32_t quantum = 4; // Set your desired quantum time
    printf("######################### START OF ROUND ROBIN #########################\n");
    executeRR(process_list, TOTAL_CREATED_PROCESSES, quantum);
    printFinal(process_list);
    printProcessSpecifics(process_list);
    printSummaryData(process_list);
    printf("######################### END OF ROUND ROBIN #########################\n");

    // Reopen the input file for the Shortest Job First scheduling
    input_file = fopen(input_file_name, "r");
    if (!input_file) {
        perror("Failed to reopen input file");
        return EXIT_FAILURE;
    }
    initializeProcesses(process_list, total_num_of_process, input_file);
    fclose(input_file);

    printf("######################### START OF SHORTEST JOB FIRST #########################\n");
    executeSJF(process_list, TOTAL_CREATED_PROCESSES);
    printFinal(process_list);
    printProcessSpecifics(process_list);
    printSummaryData(process_list);
    printf("######################### END OF SHORTEST JOB FIRST #########################\n");

    return EXIT_SUCCESS;
}

