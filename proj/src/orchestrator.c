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
    char flag[3]; //-u or -p
    char status[20];  // "waiting", "executing", "completed."
    long exec_time; // Real execution time in milliseconds
    long estimated_time; // Estimated time in milliseconds provided by the user when requests exec
    struct timeval start_time;
    struct task* next;
} Task;

Task* head = NULL;  // Head
int taskCounter = 1;  // Task ID counter
int activeTasks = 0;  // Counter for active tasks


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
    close(req_fd_write);
    close(req_fd);
    unlink(REQ_PIPE);
    unlink(RESP_PIPE);
}

void update_task_status(Task *task, const char* status) 
{
    // printf("Updating status of task %d from %s to %s\n", task->id, task->status, status); // Log de mudança de status - debugging
    strcpy(task->status, status);
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
    char buffer[300];
    int len = snprintf(buffer, sizeof(buffer), "Task ID %d completed. Command: %s, Execution Time: %ld ms\n", 
                       task->id, task->command, task->exec_time);

    if (write(fd, buffer, len) != len) 
    {
        perror("Failed to write to log file");
    }

    close(fd);
}

void calculate_and_log_duration(struct timeval start, struct timeval end, Task *task)
//void calculate_and_log_duration(struct timeval start, struct timeval end, Task *task, int pipe_write_fd)
{
    long duration = (end.tv_sec - start.tv_sec) * 1000;
    duration += (end.tv_usec - start.tv_usec) / 1000;
    task->exec_time = duration;
    
    // ssize_t bytes_written = write (pipe_write_fd, &duration, sizeof(long));
    
    // if (bytes_written == -1) 
    // {
    //     perror("Erro a escrever na pipe");
    //     exit(EXIT_FAILURE);
    // }
    // close(pipe_write_fd);

    log_task(task);
}

void write_status_to_client(int fd) 
{
    Task *current = head;
    char line[256];
    int len;
    
    write(fd, "Executing\n", strlen("Executing\n"));
    for (current = head; current != NULL; current = current->next) 
    {
        if (strcmp(current->status, "executing") == 0) 
        {
            len = snprintf(line, sizeof(line), "%d %s\n", current->id, current->command);
            write(fd, line, len);
        }
    }

    write(fd, "\nScheduled\n", strlen("\nScheduled\n"));

    for (current = head; current != NULL; current = current->next) 
    {
        if (strcmp(current->status, "waiting") == 0) 
        {
            len = snprintf(line, sizeof(line), "%d %s\n", current->id, current->command);
            write(fd, line, len);
        }
    }

    write(fd, "\nCompleted\n", strlen("\nCompleted\n"));

    for (current = head; current != NULL; current = current->next) 
    {
        if (strcmp(current->status, "completed") == 0) 
        {
            len = snprintf(line, sizeof(line), "%d %s %ld ms\n", current->id, current->command, current->exec_time);
            write(fd, line, len);
        }
    }
    write(fd, "END\n", strlen("END\n"));
}

void execute_task(Task *task, const char *output_folder) 
//void execute_task(Task *task, const char *output_folder, int pipe_write) 
{
    if (task == NULL) return;

    struct timeval end_time;
    char output_file_name[64];

    gettimeofday(&task->start_time, NULL); // Start time

    int pid = fork();
    if (pid == 0) 
    {
        // Child process: redirect stdout and stderr
        snprintf(output_file_name, sizeof(output_file_name), "../%s/task_output_%d.log", output_folder, task->id);
        int out_fd = open(output_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (out_fd == -1) 
        {
            perror("Failed to open output file");
            exit(EXIT_FAILURE);
        }
        dup2(out_fd, STDOUT_FILENO);
        dup2(out_fd, STDERR_FILENO);
        close(out_fd);

        char *args[100];
        int argc = 0;
        char *token = strtok(task->command, " ");
        while (token != NULL && argc < 99) 
        {
            args[argc++] = token;
            token = strtok(NULL, " ");
        }
        args[argc] = NULL;

        execvp(args[0], args);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } 
    else if (pid > 0) 
    {
        int status;
        waitpid(pid, &status, 0);
        gettimeofday(&end_time, NULL); // End time

        calculate_and_log_duration(task->start_time, end_time, task);
        //calculate_and_log_duration(task->start_time, end_time, task, pipe_write);
  
        // printf("Task %d completed.\n", task->id); //debugging
    } 
    else 
    {
        perror("Failed to fork");
    }
}

void execute_pipeline_task(Task *task, const char *output_folder) 
{
    if (task == NULL) return;

    int num_cmds = 0;
    char *cmds[100];
    char *command_copy = strdup(task->command);
    char *token = strtok(command_copy, "|");
    while (token != NULL && num_cmds < 99) 
    {
        cmds[num_cmds++] = strdup(token);
        token = strtok(NULL, "|");
    }
    cmds[num_cmds] = NULL;
    free(command_copy);

    int pipes[num_cmds-1][2];
    pid_t pids[num_cmds];
    struct timeval inicio, fim;

    gettimeofday(&inicio, NULL);  // Start time

    for (int i = 0; i < num_cmds; i++) {
        if (i < num_cmds - 1) {
            pipe(pipes[i]);
        }

        pids[i] = fork();
        if (pids[i] == 0) 
        {
            if (i > 0) 
            {  // Not the first command
                dup2(pipes[i-1][0], 0);
                close(pipes[i-1][0]);
                close(pipes[i-1][1]);
            }

            if (i < num_cmds - 1) 
            {  // Not the last command
                close(pipes[i][0]);
                dup2(pipes[i][1], 1);
                close(pipes[i][1]);
            } 
            else 
            {  // Last command
                char output_file_name[64];
                snprintf(output_file_name, sizeof(output_file_name), "../%s/task_output_%d.log", output_folder, task->id);
                int out_fd = open(output_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                dup2(out_fd, 1);
                close(out_fd);
            }

            char *args[100];
            int argc = 0;
            char *arg_token = strtok(cmds[i], " ");
            while (arg_token != NULL && argc < 99) 
            {
                args[argc++] = arg_token;
                arg_token = strtok(NULL, " ");
            }
            args[argc] = NULL;
            execvp(args[0], args);
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
    }

    // Close all pipes in the parent
    for (int i = 0; i < num_cmds - 1; i++) 
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes to finish
    int status;
    for (int i = 0; i < num_cmds; i++) 
    {
        waitpid(pids[i], &status, 0);
    }

    gettimeofday(&fim, NULL);  // End time
    long long tempo = (fim.tv_sec - inicio.tv_sec) * 1000;
    tempo += (fim.tv_usec - inicio.tv_usec) / 1000;

    task->exec_time = tempo;
    log_task(task);
}

void dispatch_waiting_tasks(const char *output_folder, int max_parallel_tasks)
{
        Task *current = head;

        while (current != NULL && activeTasks < max_parallel_tasks) 
        {
            if (strcmp(current->status, "waiting") == 0) 
            {
                if (activeTasks < max_parallel_tasks) 
                {
                    update_task_status(current, "executing");
                    activeTasks++;

                    //
                    // int fds[2];
                    // int return_pipe = pipe(fds);

                    // if (return_pipe == -1) 
                    // {
                    //     perror("Erro a criar pipes");
                    //     exit(EXIT_FAILURE);
                    // }

                    // printf("PIPE: [0] = %d; [1] = %d\n", fds[0], fds[1]);

                    int pid = fork();
                    if (pid == 0) 
                    {
                        //close(fds[0]);
                        if (strcmp(current->flag, "-u") == 0) 
                        {
                            execute_task(current, output_folder);
                            //execute_task(current, output_folder,fds[1]);
                            exit(0);
                        }
                        else if (strcmp(current->flag, "-p") == 0) 
                        {
                            execute_pipeline_task(current, output_folder);
                            exit(0);
                        }
                    } 
                    else if (pid > 0) 
                    {
                        //close(fds[1]);
                        // long value = 0;

                        // ssize_t bytes_read = read(fds[0], &value, sizeof(long));
                        // if (bytes_read == -1) 
                        // {
                        //     perror("Erro a ler da pipe");
                        //     exit(EXIT_FAILURE);
                        // }
                        // close(fds[0]);

                        // current->exec_time = value;
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
        new_task->id = taskCounter++; // id

        token = strtok(NULL, " "); // Skip 'execute'
        strcpy(new_task->flag, token); // flag
        
        token = strtok(NULL, " "); // estimated time
        new_task->estimated_time = atol(token);
        
        token = strtok(NULL, " ");
        strcpy(new_task->command, "");
        while (token != NULL) 
        {
            strcat(new_task->command, token);
            strcat(new_task->command, " ");
            token = strtok(NULL, " ");
        }
        
        strcpy(new_task->status, "waiting");

        if(strcmp(sched_policy,"LIFO") == 0)
        {
            //LIFO
            new_task->next = head;
            head = new_task;
        }
        
        else if(strcmp(sched_policy,"FCFS") == 0)
        {
            //FCFS order
            if (head == NULL) 
            {
                head = new_task;
                new_task->next = NULL;
            } 
            else 
            {
                Task *current = head;
                while (current->next != NULL) 
                {
                    current = current->next;
                }
                current->next = new_task;
                new_task->next = NULL;
            }
        }

        else if(strcmp(sched_policy,"SJF") == 0)
        {
            // SJF
            if (head == NULL || new_task->estimated_time < head->estimated_time) 
            {
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
                new_task->next = current->next;
                current->next = new_task;
            }

        }
        else
        {
            perror("Sched policy not supported\n");
            return;
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
            activeTasks++;

            
            // int fds[2];
            // int return_pipe = pipe(fds);

            // if (return_pipe == -1) 
            // {
            //     perror("Erro a criar pipes");
            //     exit(EXIT_FAILURE);
            // }

            // printf("PIPE: [0] = %d; [1] = %d\n", fds[0], fds[1]);

            // Fork a process to execute the task
            int pid = fork();
            
            if (pid == 0) 
            {
                //close(fds[0]);

                if (strcmp(new_task->flag, "-u") == 0)
                {
                execute_task(new_task, output_folder);
                //execute_task(new_task, output_folder,fds[1]);
                exit(0);
                }

                else if (strcmp(new_task->flag, "-p") == 0) 
                {
                    execute_pipeline_task(new_task, output_folder);
                    exit(0);
                }
            } 
            else if (pid > 0) 
            {
                // //close(fds[1]);
                // long value = 0;

                // ssize_t bytes_read = read(fds[0], &value, sizeof(long));
                // if (bytes_read == -1) 
                // {
                //     perror("Erro a ler da pipe");
                //     exit(EXIT_FAILURE);
                // }
                // close(fds[0]);

                // new_task->exec_time = value;

                new_task->pid = pid;
            } 
            else 
            {
                perror("Failed to fork");
            }
        }
    } 
    else if (strcmp(token, "status") == 0) 
    {
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
        clean_up(-1, -1);
        exit(0);
    }
}


void handle_requests(const char *output_folder, const char *sched_policy, int max_parallel_tasks) 
{
    int req_fd = open(REQ_PIPE, O_RDONLY);
    int req_fd_write = open(REQ_PIPE, O_WRONLY);

    if (req_fd == -1 || req_fd_write == -1) 
    {
        perror("Failed to open request or response pipe");
        return;
    }
    
    char command[300];

    memset(command, 0, sizeof(command));
    while (read(req_fd, command, sizeof(command) - 1) > 0) 
    {
        check_child_processes();
        dispatch_waiting_tasks(output_folder, max_parallel_tasks);
        
        command[sizeof(command) - 1] = '\0';
        handle_command(command, output_folder, max_parallel_tasks, sched_policy);
        memset(command, 0, sizeof(command));
    }
    
    clean_up(req_fd, req_fd_write);
}

#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[]) 
{
    if (argc != 4) 
    {
    exit(EXIT_FAILURE);
    }

    const char *output_folder = argv[1];
    int max_parallel_tasks = atoi(argv[2]);
    const char *sched_policy = argv[3];
    
    setup_pipes();
    handle_requests(output_folder, sched_policy, max_parallel_tasks);
    unlink(REQ_PIPE);
    unlink(RESP_PIPE);
    return 0;
}