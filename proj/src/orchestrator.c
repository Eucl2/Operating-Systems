#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

#define REQ_PIPE "client_to_orchestrator"
#define RESP_PIPE "orchestrator_to_client"
#define LOG_FILE "completed_tasks.log"

typedef struct task {
    int id;
    char command[256];
    char status[20];  // "waiting", "executing", "completed"
    struct timeval start_time;
    struct task* next;
} Task;

Task* head = NULL;  // Head of the linked list for task queue
int taskCounter = 1;  // Task ID counter

void setup_pipes() 
{
    mkfifo(REQ_PIPE, 0666);
    mkfifo(RESP_PIPE, 0666);
}

void log_task(Task *task, long duration) 
{
    int fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd == -1) {
        perror("Failed to open log file");
        return;
    }
    dprintf(fd, "Task ID %d completed. Command: %s, Execution Time: %ld ms\n", task->id, task->command, duration);
    close(fd);
}

void update_task_status(Task *task, const char* status) 
{
    strcpy(task->status, status);
}

void execute_task(Task *task) 
{
    printf("Task to execute: %s\n", task->command); //debugging
    if (task == NULL) return;
    struct timeval end_time;
    long duration;

    gettimeofday(&task->start_time, NULL); // Start time
    update_task_status(task, "executing");

    int pid = fork();
    if (pid == 0) 
    { 
        // Child process
        execlp("/bin/sh", "sh", "-c", task->command, NULL);
        exit(EXIT_FAILURE);
    } 
    else if (pid > 0) 
    { 
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        gettimeofday(&end_time, NULL); // End time

        duration = (end_time.tv_sec - task->start_time.tv_sec) * 1000; //separated func maybe
        duration += (end_time.tv_usec - task->start_time.tv_usec) / 1000;

        log_task(task, duration); // Log the task with execution time
        update_task_status(task, "completed");
    } 
    else 
    {
        perror("Failed to fork");
    }
}

void handle_requests() {
    int req_fd = open(REQ_PIPE, O_RDONLY);
    char command[256];

    while (read(req_fd, command, sizeof(command)) > 0) {
        printf("Received command: %s\n", command);
        char *token = strtok(command, " ");
        if (strcmp(token, "execute") == 0) {
            Task *new_task = malloc(sizeof(Task));
            new_task->id = taskCounter++;

            // Skip 'execute' and scheduling flags
            token = strtok(NULL, " "); // Skip 'execute'
            token = strtok(NULL, " "); // Skip '-u' or '-p'
            token = strtok(NULL, " "); // Skip duration

            // Start constructing the command to be executed
            strcpy(new_task->command, "");

            while (token != NULL) {
                strcat(new_task->command, token);
                strcat(new_task->command, " ");
                token = strtok(NULL, " ");
            }

            strcpy(new_task->status, "waiting");
            new_task->next = head;
            head = new_task;

            execute_task(new_task);
        } else if (strcmp(token, "status") == 0) {
            printf("Status not handled yet");
        }
    }

    close(req_fd);
}


int main() 
{
    setup_pipes();
    handle_requests();
    unlink(REQ_PIPE);
    unlink(RESP_PIPE);
    return 0;
}


