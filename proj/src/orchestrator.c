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
    pid_t pid; //for waiting tasks.
    char command[256];
    char status[20];  // "waiting", "executing", "completed."
    long exec_time; // Real execution time in milliseconds
    long estimated_time; // Estimated time in milliseconds provided by the user when requests exec
    struct timeval start_time;
    struct task* next;
} Task;

Task* head = NULL;  // Head of the linked list for task queue
int taskCounter = 1;  // Task ID counter
int activeTasks = 0;  // Global counter for active tasks -> still not used


Task* find_task_by_pid(pid_t pid) 
{
    Task *current = head;
    while (current != NULL) 
    {
        if (current->pid == pid) 
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}


void setup_pipes() 
{
    mkfifo(REQ_PIPE, 0666);
    mkfifo(RESP_PIPE, 0666);
}

void clean_up(int req_fd, int req_fd_write) 
{
    //tbu when shutting down orchestrator
    close(req_fd_write);
    close(req_fd);
    unlink(REQ_PIPE);
    unlink(RESP_PIPE);

}

void update_task_status(Task *task, const char* status) 
{
    printf("Updating status of task %d from %s to %s\n", task->id, task->status, status); // Log de mudança de status
    strcpy(task->status, status);
    printf("Updated task %d 's status to: %s\n", task->id, task->status);
}

void process_task_completion(Task *task)
{
    update_task_status(task, "completed");
    activeTasks--;
}

void check_child_processes() //after execution...
{
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) 
    {
        if (WIFEXITED(status) || WIFSIGNALED(status)) 
        {
            Task *completedTask = find_task_by_pid(pid);
            
            if (completedTask != NULL) 
            {
                process_task_completion(completedTask);
            }
        }
    }
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
    printf("writing status to client...\n");
    Task *current = head;
    char line[256]; // Buffer para linhas individuais
    int len; // Comprimento da string para enviar

    // Escrever cabeçalhos de cada parte
    
    write(fd, "Executing\n", strlen("Executing\n"));
    printf("Status - writing execution...\n"); //debugging
    for (current = head; current != NULL; current = current->next) 
    {
        printf("STATUS: %s : %s\n", current->command, current->status ); //debug
        if (strcmp(current->status, "executing") == 0) 
        {
            len = snprintf(line, sizeof(line), "%d %s\n", current->id, current->command); //should be using write() ?
            write(fd, line, len);
        }
    }

    write(fd, "\nScheduled\n", strlen("\nScheduled\n"));
    printf("Status - writing scheduled...\n"); //debugging
    for (current = head; current != NULL; current = current->next) 
    {
        if (strcmp(current->status, "waiting") == 0) 
        {
            len = snprintf(line, sizeof(line), "%d %s\n", current->id, current->command); //should be using write() ?
            write(fd, line, len);
        }
    }

    write(fd, "\nCompleted\n", strlen("\nCompleted\n"));
    printf("Status - writing completed...\n"); //debugging
    for (current = head; current != NULL; current = current->next) 
    {
        if (strcmp(current->status, "completed") == 0) 
        {
            //should be using write() ?
            len = snprintf(line, sizeof(line), "%d %s %ld ms\n", current->id, current->command, current->exec_time);
            write(fd, line, len);
        }
    }

    // Indicar fim da transmissão - alterar? //linha em branco - alterar
    write(fd, "END\n", strlen("END\n"));
}

void execute_task(Task *task, const char *output_folder) 
{
    if (task == NULL) return;
    printf("Starting execution of task: %d\n", task->id);
    
    struct timeval end_time;
    
    char output_file_name[64]; //required to send stdout e perror to text file with task id

    gettimeofday(&task->start_time, NULL); // Start time

    int pid = fork();
    if (pid == 0) 
    { 
        // Child process: redirect stdout and stderr
        snprintf(output_file_name, sizeof(output_file_name), "../%s/task_output_%d.log",output_folder, task->id);
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
        perror("execlp failed");
        exit(EXIT_FAILURE);
    } 
    else if (pid > 0) 
    { 
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        gettimeofday(&end_time, NULL); // End time

        calculate_and_log_duration(task->start_time, end_time, task);
  
        printf("Task %d completed.\n", task->id);
        //update_task_status(task, "completed");
    } 
    else 
    {
        perror("Failed to fork");
    }
}

void dispatch_waiting_tasks(const char *output_folder, int max_parallel_tasks)
{ //goal: tasks waiting to be executed
        Task *current = head;

        while (current != NULL && activeTasks < max_parallel_tasks) 
        {
            if (strcmp(current->status, "waiting") == 0) 
            {
                if (activeTasks < max_parallel_tasks) 
                {
                    update_task_status(current, "executing");
                    activeTasks++;
                    int pid = fork();
                    if (pid == 0) 
                    {
                        execute_task(current, output_folder);
                        exit(0);
                    } 
                    else if (pid > 0) 
                    {
                        current->pid = pid;
                    } 
                    else 
                    {
                        perror("Failed to fork");
                    }
                }
            }
            current = current->next;
        }
    }

void handle_command(char* command, const char *output_folder, int max_parallel_tasks, const char *sched_policy) 
{
    char *token = strtok(command, " ");
    if (strcmp(token, "execute") == 0) 
    {
        Task *new_task = (Task*)malloc(sizeof(Task));
        if (new_task == NULL) 
        {
            perror("Failed to allocate memory for new task");
            return;
        }
        new_task->id = taskCounter++;  // Allocate ID in parent

        // Skip 'execute' and scheduling flags
        token = strtok(NULL, " "); // Skip 'execute'
        token = strtok(NULL, " "); // Skip '-u' or '-p'
        new_task->estimated_time = atol(token); // Convert and store estimated time
        printf(">>>>>>>>>time:%li\n", new_task->estimated_time); //debug
        token = strtok(NULL, " ");
        strcpy(new_task->command, "");
        while (token != NULL) 
        {
            strcat(new_task->command, token);
            strcat(new_task->command, " ");
            token = strtok(NULL, " ");
        }
        
        strcpy(new_task->status, "waiting");
        
        //LIFO
        // new_task->next = head;
        //head = new_task;
        
        if(strcmp(sched_policy,"FCFS") == 0)
        {
            //FCFS order
            if (head == NULL) 
            {
                head = new_task; // If list is empty, new task becomes head
                new_task->next = NULL;
            } 
            else 
            {
                Task *current = head;
                while (current->next != NULL) 
                {
                    current = current->next; // end of list
                }
                current->next = new_task; // Append new task at the end to maintain FCFS :)
                new_task->next = NULL;
            }
        }

        else if(strcmp(sched_policy,"SJF") == 0)
        {
        
            // SJF: insert based on exp time
            if (head == NULL || new_task->estimated_time < head->estimated_time) 
            {
                //insert on head
                new_task->next = head;
                head = new_task;
            }
            else 
            {
                // find where to insert
                Task *current = head;
                while (current->next != NULL && current->next->estimated_time <= new_task->estimated_time) 
                {
                    current = current->next;
                }
                // insert there
                new_task->next = current->next;
                current->next = new_task;
            }

        }
        
        int resp_fd = open(RESP_PIPE, O_WRONLY);
        if (resp_fd == -1) 
        {
            perror("Failed to open response pipe");
            return;
        }
        char response[50];
        sprintf(response, "%d", new_task->id);
        write(resp_fd, response, strlen(response));
        close(resp_fd);

        if (activeTasks < max_parallel_tasks) 
        {
            update_task_status(new_task, "executing");
            activeTasks++;  // Incrementar o contador de tarefas ativas

            // Fork a process to execute the task
            int pid = fork();
            
            if (pid == 0) 
            {
                //child process to execute
                execute_task(new_task, output_folder);
                exit(0);  // Garante que o processo filho termine após a execução da tarefa
            } 
            else if (pid > 0) 
            {
                new_task->pid = pid; //so that we can later access the tasks that...hmmm.... are waiting :)
            } 
            else 
            {
                perror("Failed to fork");
            }
        }
        else printf ("Max parallel tasks reached. WAIT!!!\n");
    } 
    else if (strcmp(token, "status") == 0) 
    {
        printf("Status request received.\n"); //debugging
        int resp_fd = open(RESP_PIPE, O_WRONLY);
        if (resp_fd == -1) 
        {
            perror("Error opening response pipe");
            return;
        }
        write_status_to_client(resp_fd);
        close(resp_fd);
    } 
    else if (strcmp(token, "shutdown") == 0) 
    {
        printf("Shutting down orchestrator...\n");
        clean_up(-1, -1);  // ????????????????? passar os fd?
        exit(0);
    }
}


void handle_requests(const char *output_folder, const char *sched_policy, int max_parallel_tasks) 
{
    printf("Orchestrator started with policy %s and max parallel tasks: %d\n", sched_policy, max_parallel_tasks);

    int req_fd = open(REQ_PIPE, O_RDONLY);
    int req_fd_write = open(REQ_PIPE, O_WRONLY); // Keep the pipe open

    if (req_fd == -1 || req_fd_write == -1) 
    {
        perror("Failed to open request or response pipe");
        return;
    }


    printf("FCFS execution\n"); //debug
    
    char command[300];
    while (read(req_fd, command, sizeof(command) - 1) > 0) 
    {
        check_child_processes(); //no blocking while waiting for child to be completed
        dispatch_waiting_tasks(output_folder, max_parallel_tasks); //waiting to execute
        printf(">Comando a executar: %s\n", command);
        command[sizeof(command) - 1] = '\0'; // Ensure null-terminated
        handle_command(command, output_folder, max_parallel_tasks, sched_policy);
        memset(command, 0, sizeof(command)); // Clear buffer
    }
    
    clean_up(req_fd, req_fd_write);
}

#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[]) 
{
    if (argc != 4) 
    {
    printf("Usage: ./orchestrator output_folder parallel-tasks sched-policy\n");
    exit(EXIT_FAILURE);
    }

    const char *output_folder = argv[1];
    int max_parallel_tasks = atoi(argv[2]); // Converte o número de tarefas paralelas para int -> not used
    const char *sched_policy = argv[3]; //-> not used
    
    setup_pipes(); // should we be doing mkfifo for both pipes here?
    handle_requests(output_folder, sched_policy, max_parallel_tasks);
    unlink(REQ_PIPE);
    unlink(RESP_PIPE);
    return 0;
}
