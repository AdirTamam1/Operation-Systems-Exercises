// Adir Tamam 318936507
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <limits.h>
#include <fcntl.h>
#include <stdbool.h>

#define MAX_PROCESSES 1000
#define MAX_LINE_LENGTH 256
#define MAX_NAME_LENGTH 51
#define MAX_DESC_LENGTH 101

typedef struct {
    char name[MAX_NAME_LENGTH];
    char description[MAX_DESC_LENGTH];
    int arrival_time;
    int burst_time;
    int priority;
    int remaining_time;
    int waiting_time;
    int completion_time;
    int start_time;
    int original_order;
} Process;


typedef struct {
    Process* data[MAX_PROCESSES];
    int front, rear;
} Queue;


// Global variables for signal handling
volatile sig_atomic_t alarm_fired = 0;

// Signal handler for alarm
void alarm_handler(int sig) {
    alarm_fired = 1;
}

// Function to simulate time passage using signals (required for assignment)
void simulate_time(int duration) {
    if (duration <= 0) return;

    // Set up signal handler using sigaction (satisfies signal requirement)
    struct sigaction sa;
    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    alarm_fired = 0;
    alarm(duration);

    // Wait for alarm signal
    while (!alarm_fired) {
        pause();
    }
}

// Parse CSV line
int parse_csv_line(char *line, Process *proc, int order) {
    char *token;
    char *line_copy = strdup(line);

    // Remove newline if present
    line_copy[strcspn(line_copy, "\n")] = 0;

    // Parse name
    token = strtok(line_copy, ",");
    if (!token) return 0;
    strncpy(proc->name, token, MAX_NAME_LENGTH - 1);
    proc->name[MAX_NAME_LENGTH - 1] = '\0';

    // Parse description
    token = strtok(NULL, ",");
    if (!token) return 0;
    strncpy(proc->description, token, MAX_DESC_LENGTH - 1);
    proc->description[MAX_DESC_LENGTH - 1] = '\0';

    // Parse arrival time
    token = strtok(NULL, ",");
    if (!token) return 0;
    proc->arrival_time = atoi(token);

    // Parse burst time
    token = strtok(NULL, ",");
    if (!token) return 0;
    proc->burst_time = atoi(token);
    proc->remaining_time = proc->burst_time;

    // Parse priority
    token = strtok(NULL, ",");
    if (!token) return 0;
    proc->priority = atoi(token);

    proc->original_order = order;
    proc->waiting_time = 0;
    proc->completion_time = 0;
    proc->start_time = -1;

    free(line_copy);
    return 1;
}

// Read processes from CSV file using low-level system calls
int read_processes(const char *filename, Process processes[]) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    int count = 0;
    int line_pos = 0;
    ssize_t bytes_read;
    char c;

    while (count < MAX_PROCESSES) {
        line_pos = 0;

        // Read line character by character using read()
        while ((bytes_read = read(fd, &c, 1)) > 0 && c != '\n' && line_pos < MAX_LINE_LENGTH - 1) {
            line[line_pos++] = c;
        }

        // End of file reached
        if (bytes_read <= 0 && line_pos == 0) {
            break;
        }

        // Null terminate the line
        line[line_pos] = '\0';

        // Parse the line if it's not empty
        if (line_pos > 0 && parse_csv_line(line, &processes[count], count)) {
            count++;
        }

        // If we hit EOF, break
        if (bytes_read <= 0) {
            break;
        }
    }

    close(fd);
    return count;
}

// Comparison functions for sorting
int compare_fcfs(const void *a, const void *b) {
    Process *pa = (Process *)a;
    Process *pb = (Process *)b;

    if (pa->arrival_time != pb->arrival_time)
        return pa->arrival_time - pb->arrival_time;
    return pa->original_order - pb->original_order;
}

int compare_sjf(const void *a, const void *b) {
    Process *pa = (Process *)a;
    Process *pb = (Process *)b;

    if (pa->burst_time != pb->burst_time)
        return pa->burst_time - pb->burst_time;
    if (pa->arrival_time != pb->arrival_time)
        return pa->arrival_time - pb->arrival_time;
    return pa->original_order - pb->original_order;
}

int compare_priority(const void *a, const void *b) {
    Process *pa = (Process *)a;
    Process *pb = (Process *)b;

    if (pa->priority != pb->priority)
        return pa->priority - pb->priority;
    if (pa->arrival_time != pb->arrival_time)
        return pa->arrival_time - pb->arrival_time;
    return pa->original_order - pb->original_order;
}

// FCFS Scheduling
void schedule_fcfs(Process processes[], int n) {
    Process temp[MAX_PROCESSES];
    memcpy(temp, processes, n * sizeof(Process));
    qsort(temp, n, sizeof(Process), compare_fcfs);

    printf("══════════════════════════════════════════════\n");
    printf(">> Scheduler Mode : FCFS\n");
    printf(">> Engine Status  : Initialized\n");
    printf("──────────────────────────────────────────────\n\n");

    int current_time = 0;
    double total_waiting_time = 0;

    for (int i = 0; i < n; i++) {
        // Handle idle time
        if (current_time < temp[i].arrival_time) {
            printf("%d → %d: Idle.\n", current_time, temp[i].arrival_time);
            simulate_time(temp[i].arrival_time - current_time);
            current_time = temp[i].arrival_time;
        }

        // Calculate waiting time
        temp[i].waiting_time = current_time - temp[i].arrival_time;
        total_waiting_time += temp[i].waiting_time;

        // Execute process
        printf("%d → %d: %s Running %s.\n",
               current_time, current_time + temp[i].burst_time,
               temp[i].name, temp[i].description);

        simulate_time(temp[i].burst_time);
        current_time += temp[i].burst_time;
        temp[i].completion_time = current_time;
    }

    double avg_waiting_time = total_waiting_time / n;
    printf("\n──────────────────────────────────────────────\n");
    printf(">> Engine Status  : Completed\n");
    printf(">> Summary        :\n");
    printf("   └─ Average Waiting Time : %.2f time units\n", avg_waiting_time);
    printf(">> End of Report\n");
    printf("══════════════════════════════════════════════\n\n");
}

// SJF Scheduling (Non-preemptive)
void schedule_sjf(Process processes[], int n) {
    Process temp[MAX_PROCESSES];
    memcpy(temp, processes, n * sizeof(Process));

    printf("══════════════════════════════════════════════\n");
    printf(">> Scheduler Mode : SJF\n");
    printf(">> Engine Status  : Initialized\n");
    printf("──────────────────────────────────────────────\n\n");

    int current_time = 0;
    double total_waiting_time = 0;
    int completed = 0;
    int is_completed[MAX_PROCESSES] = {0};

    while (completed < n) {
        int shortest_idx = -1;
        int shortest_burst = INT_MAX;

        // Find the shortest job among arrived processes
        for (int i = 0; i < n; i++) {
            if (!is_completed[i] && temp[i].arrival_time <= current_time) {
                if (temp[i].burst_time < shortest_burst ||
                    (temp[i].burst_time == shortest_burst &&
                     (shortest_idx == -1 || temp[i].arrival_time < temp[shortest_idx].arrival_time ||
                      (temp[i].arrival_time == temp[shortest_idx].arrival_time &&
                       temp[i].original_order < temp[shortest_idx].original_order)))) {
                    shortest_burst = temp[i].burst_time;
                    shortest_idx = i;
                }
            }
        }

        if (shortest_idx == -1) {
            // No process available, find next arrival
            int next_arrival = INT_MAX;
            for (int i = 0; i < n; i++) {
                if (!is_completed[i] && temp[i].arrival_time > current_time) {
                    if (temp[i].arrival_time < next_arrival) {
                        next_arrival = temp[i].arrival_time;
                    }
                }
            }

            printf("%d → %d: Idle.\n", current_time, next_arrival);
            simulate_time(next_arrival - current_time);
            current_time = next_arrival;
        } else {
            // Execute the shortest job
            temp[shortest_idx].waiting_time = current_time - temp[shortest_idx].arrival_time;
            total_waiting_time += temp[shortest_idx].waiting_time;

            printf("%d → %d: %s Running %s.\n",
                   current_time, current_time + temp[shortest_idx].burst_time,
                   temp[shortest_idx].name, temp[shortest_idx].description);

            simulate_time(temp[shortest_idx].burst_time);
            current_time += temp[shortest_idx].burst_time;
            temp[shortest_idx].completion_time = current_time;
            is_completed[shortest_idx] = 1;
            completed++;
        }
    }

    double avg_waiting_time = total_waiting_time / n;
    printf("\n──────────────────────────────────────────────\n");
    printf(">> Engine Status  : Completed\n");
    printf(">> Summary        :\n");
    printf("   └─ Average Waiting Time : %.2f time units\n", avg_waiting_time);
    printf(">> End of Report\n");
    printf("══════════════════════════════════════════════\n\n");
}

// Priority Scheduling (Non-preemptive)
void schedule_priority(Process processes[], int n) {
    Process temp[MAX_PROCESSES];
    memcpy(temp, processes, n * sizeof(Process));

    printf("══════════════════════════════════════════════\n");
    printf(">> Scheduler Mode : Priority\n");
    printf(">> Engine Status  : Initialized\n");
    printf("──────────────────────────────────────────────\n\n");

    int current_time = 0;
    double total_waiting_time = 0;
    int completed = 0;
    int is_completed[MAX_PROCESSES] = {0};

    while (completed < n) {
        int highest_priority_idx = -1;
        int highest_priority = INT_MAX;

        // Find the highest priority job among arrived processes
        for (int i = 0; i < n; i++) {
            if (!is_completed[i] && temp[i].arrival_time <= current_time) {
                if (temp[i].priority < highest_priority ||
                    (temp[i].priority == highest_priority &&
                     (highest_priority_idx == -1 || temp[i].arrival_time < temp[highest_priority_idx].arrival_time ||
                      (temp[i].arrival_time == temp[highest_priority_idx].arrival_time &&
                       temp[i].original_order < temp[highest_priority_idx].original_order)))) {
                    highest_priority = temp[i].priority;
                    highest_priority_idx = i;
                }
            }
        }

        if (highest_priority_idx == -1) {
            // No process available, find next arrival
            int next_arrival = INT_MAX;
            for (int i = 0; i < n; i++) {
                if (!is_completed[i] && temp[i].arrival_time > current_time) {
                    if (temp[i].arrival_time < next_arrival) {
                        next_arrival = temp[i].arrival_time;
                    }
                }
            }

            printf("%d → %d: Idle.\n", current_time, next_arrival);
            simulate_time(next_arrival - current_time);
            current_time = next_arrival;
        } else {
            // Execute the highest priority job
            temp[highest_priority_idx].waiting_time = current_time - temp[highest_priority_idx].arrival_time;
            total_waiting_time += temp[highest_priority_idx].waiting_time;

            printf("%d → %d: %s Running %s.\n",
                   current_time, current_time + temp[highest_priority_idx].burst_time,
                   temp[highest_priority_idx].name, temp[highest_priority_idx].description);

            simulate_time(temp[highest_priority_idx].burst_time);
            current_time += temp[highest_priority_idx].burst_time;
            temp[highest_priority_idx].completion_time = current_time;
            is_completed[highest_priority_idx] = 1;
            completed++;
        }
    }

    double avg_waiting_time = total_waiting_time / n;
    printf("\n──────────────────────────────────────────────\n");
    printf(">> Engine Status  : Completed\n");
    printf(">> Summary        :\n");
    printf("   └─ Average Waiting Time : %.2f time units\n", avg_waiting_time);
    printf(">> End of Report\n");
    printf("══════════════════════════════════════════════\n\n");
}


void init_queue(Queue* q) {
    q->front = q->rear = 0;
}

bool is_empty(Queue* q) {
    return q->front == q->rear;
}

void enqueue(Queue* q, Process* p) {
    q->data[q->rear++] = p;
}

Process* dequeue(Queue* q) {
    return q->data[q->front++];
}


int compare_processes(const void* a, const void* b) {
    Process* p1 = (Process*)a;
    Process* p2 = (Process*)b;
    if (p1->arrival_time != p2->arrival_time)
        return p1->arrival_time - p2->arrival_time;
    return p1->original_order - p2->original_order;
}
void schedule_round_robin(Process processes[], int n, int timeQuantum) {
    printf("══════════════════════════════════════════════\n");
    printf(">> Scheduler Mode : Round Robin\n");
    printf(">> Engine Status  : Initialized\n");
    printf("──────────────────────────────────────────────\n\n");

    int current_time = 0;
    int completed = 0;

    // Total turnaround time is sum of all burst times
    int total_turnaround_time = 0;
    for (int i = 0; i < n; i++) {
        total_turnaround_time += processes[i].burst_time;
        processes[i].remaining_time = processes[i].burst_time; // Initialize remaining time
        processes[i].start_time = -1; // Mark start time as not started
    }

    Queue queue;
    init_queue(&queue);

    bool added[MAX_PROCESSES] = { false };

    // Enqueue all processes that arrive at time 0
    for (int i = 0; i < n; i++) {
        if (processes[i].arrival_time <= current_time) {
            enqueue(&queue, &processes[i]);
            added[i] = true;
        }
    }

    while (completed < n) {
        if (is_empty(&queue)) {
            // CPU is idle, jump to next process arrival time
            int next_arrival = INT_MAX;
            for (int i = 0; i < n; i++) {
                if (!added[i] && processes[i].arrival_time < next_arrival) {
                    next_arrival = processes[i].arrival_time;
                }
            }

            // Print idle time
            printf("%d → %d: Idle.\n", current_time, next_arrival);
            simulate_time(next_arrival - current_time);
            total_turnaround_time += next_arrival - current_time;
            current_time = next_arrival;

            // Enqueue processes that arrive now
            for (int i = 0; i < n; i++) {
                if (!added[i] && processes[i].arrival_time <= current_time) {
                    enqueue(&queue, &processes[i]);
                    added[i] = true;
                }
            }
        } else {
            // Dequeue next process
            Process* p = dequeue(&queue);

            if (p->start_time == -1) {
                p->start_time = current_time;
            }

            int exec_time = (p->remaining_time < timeQuantum) ? p->remaining_time : timeQuantum;

            printf("%d → %d: %s Running %s.\n", current_time, current_time + exec_time, p->name, p->description);
            simulate_time(exec_time);

            int end_time = current_time + exec_time;
            p->remaining_time -= exec_time;

            Process* to_enqueue[MAX_PROCESSES];
            int enqueue_count = 0;

            // Add newly arrived processes before or at end_time
            for (int i = 0; i < n; i++) {
                if (!added[i] && processes[i].arrival_time <= end_time) {
                    to_enqueue[enqueue_count++] = &processes[i];
                    added[i] = true;
                }
            }

            // If process not finished, re-add it
            if (p->remaining_time > 0) {
                to_enqueue[enqueue_count++] = p;
            } else {
                p->completion_time = end_time;
                p->waiting_time = p->completion_time - p->arrival_time - p->burst_time;
                completed++;
            }

            // Sort by original order to prioritize lower index first on tie
            for (int i = 0; i < enqueue_count - 1; i++) {
                for (int j = i + 1; j < enqueue_count; j++) {
                    if (to_enqueue[i]->original_order > to_enqueue[j]->original_order) {
                        Process* temp = to_enqueue[i];
                        to_enqueue[i] = to_enqueue[j];
                        to_enqueue[j] = temp;
                    }
                }
            }

            // Enqueue all
            for (int i = 0; i < enqueue_count; i++) {
                enqueue(&queue, to_enqueue[i]);
            }

            current_time = end_time;
        }
    }

    printf("\n──────────────────────────────────────────────\n");
    printf(">> Engine Status  : Completed\n");
    printf(">> Summary        :\n");
    printf("   └─ Total Turnaround Time : %d time units\n\n", total_turnaround_time);
    printf(">> End of Report\n");
    printf("══════════════════════════════════════════════\n\n");
}




// Main CPU Scheduler function
void runCPUScheduler(char* processesCsvFilePath, int timeQuantum) {
    Process processes[MAX_PROCESSES];
    int n = read_processes(processesCsvFilePath, processes);

    if (n <= 0) {
        fprintf(stderr, "Error: Could not read processes from file\n");
        return;
    }

    // Flush output to prevent buffering issues that cause duplication
    fflush(stdout);
    fflush(stderr);

    // Run all four scheduling algorithms
    schedule_fcfs(processes, n);
    fflush(stdout);  // Ensure clean output between algorithms

    schedule_sjf(processes, n);
    fflush(stdout);

    schedule_priority(processes, n);
    fflush(stdout);

    schedule_round_robin(processes, n, timeQuantum);
    fflush(stdout);
}