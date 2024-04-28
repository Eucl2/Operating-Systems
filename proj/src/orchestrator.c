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
    long exec_time; // in milliseconds
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

void log_task(Task *task) 
{
    int fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd == -1) 
    {
        perror("Failed to open log file");
        return;
    }
    //probably should be using write() func instead
    dprintf(fd, "Task ID %d completed. Command: %s, Execution Time: %ld ms\n", task->id, task->command, task->exec_time);
    close(fd);
}

void calculate_and_log_duration(struct timeval start, struct timeval end, Task *task)
{
    //func to calculate time. Adapt code.
    //not used for now
    long duration = (end.tv_sec - start.tv_sec) * 1000;
    duration += (end.tv_usec - start.tv_usec) / 1000;
    task->exec_time = duration;
    log_task(task);  // Log
}

void write_status_to_client(int fd) 
{
    Task *current = head;
    char line[256]; // Buffer para linhas individuais
    int len; // Comprimento da string para enviar

    // Escrever cabeçalhos de cada parte
    
    write(fd, "Executing\n", strlen("Executing\n"));
    printf("Status - writing execution..."); //debugging
    for (current = head; current != NULL; current = current->next) 
    {
        if (strcmp(current->status, "executing") == 0) 
        {
            len = snprintf(line, sizeof(line), "%d %s\n", current->id, current->command); //should be using write() ?
            write(fd, line, len);
        }
    }

    write(fd, "Scheduled\n", strlen("Scheduled\n"));
    printf("Status - writing acheduled..."); //debugging
    for (current = head; current != NULL; current = current->next) 
    {
        if (strcmp(current->status, "waiting") == 0) 
        {
            len = snprintf(line, sizeof(line), "%d %s\n", current->id, current->command); //should be using write() ?
            write(fd, line, len);
        }
    }

    write(fd, "Completed\n", strlen("Completed\n"));
    printf("Status - writing completed..."); //debugging
    for (current = head; current != NULL; current = current->next) 
    {
        if (strcmp(current->status, "completed") == 0) 
        {
            //deveria mostrar o tempo que demorrou a executar, teremos que guardá-lo na struct. Por agora fica o start time
            //should be using write() ?
            len = snprintf(line, sizeof(line), "%d %s %ld ms\n", current->id, current->command, current->exec_time);
            write(fd, line, len);
        }
    }

    // Indicar fim da transmissão -alterar?
    write(fd, "END\n", strlen("END\n"));
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
    
    char output_file_name[64]; //required to send stdout e perror to text file with task id

    gettimeofday(&task->start_time, NULL); // Start time
    update_task_status(task, "executing");

    int pid = fork();
    if (pid == 0) 
    { 
        // Child process: redirect stdout and stderr
        snprintf(output_file_name, sizeof(output_file_name), "task_output_%d.log", task->id);
        int out_fd = open(output_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (out_fd == -1) 
        {
            perror("Failed to open output file");
            exit(EXIT_FAILURE);
        }
        dup2(out_fd, STDOUT_FILENO);
        dup2(out_fd, STDERR_FILENO);
        close(out_fd);

        execlp("/bin/sh", "sh", "-c", task->command, NULL);
        exit(EXIT_FAILURE);
    } 
    else if (pid > 0) 
    { 
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        gettimeofday(&end_time, NULL); // End time

        calculate_and_log_duration(task->start_time, end_time, task);

        update_task_status(task, "completed");
    } 
    else 
    {
        perror("Failed to fork");
    }
}

void handle_requests() 
{
    int req_fd = open(REQ_PIPE, O_RDONLY);
    char command[256];

    while (read(req_fd, command, sizeof(command)) > 0) 
    {
        printf("Received command: %s\n", command);
        char *token = strtok(command, " ");
        if (strcmp(token, "execute") == 0) 
        {
            Task *new_task = malloc(sizeof(Task));
            if(new_task == NULL)
            {
                //error allocating memory
                continue;
            }
            new_task->id = taskCounter++;

            // Skip 'execute' and scheduling flags
            token = strtok(NULL, " "); // Skip 'execute'
            token = strtok(NULL, " "); // Skip '-u' or '-p'
            token = strtok(NULL, " "); // Skip duration

            // Start constructing the command to be executed
            strcpy(new_task->command, "");

            while (token != NULL) 
            {
                strcat(new_task->command, token);
                strcat(new_task->command, " ");
                token = strtok(NULL, " ");
            }

            strcpy(new_task->status, "waiting");
            new_task->next = head;
            head = new_task;

            execute_task(new_task);
        } 
        else if (strcmp(token, "status") == 0) 
        {
            int resp_fd = open(RESP_PIPE, O_WRONLY);
            
            if (resp_fd == -1) 
            {
                perror("Error opening response pipe");
                exit(1);
            }

            write_status_to_client(resp_fd);
            close(resp_fd); //fechar no final
        }
    }

    close(req_fd);
}

#include <string.h>
#include <unistd.h>

int main() 
{
    setup_pipes(); // should we be doing mkfifo for both pipes here?
    handle_requests();
    unlink(REQ_PIPE);
    unlink(RESP_PIPE);
    return 0;
}


