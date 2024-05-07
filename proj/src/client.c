#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define REQ_PIPE "client_to_orchestrator"
#define RESP_PIPE "orchestrator_to_client"

int is_integer(const char *str) 
{
    if (*str == '\0') return 0;
    for (; *str != '\0'; str++) 
    {
        if (*str < '0' || *str > '9') return 0;
    }

    return 1;  // Todos os caracteres são dígitos válidos
}

void send_request(char* command) 
{
    char output_message[128];
    if (command != NULL) 
    {
        // printf("Sending [ %s ] request to orchestrator... \n", command); //debugging
    } 
    else 
    {
        // printf("Error: Command is NULL\n"); //debugging
    }

    int fd_req, fd_resp;
    char task_id[100] = {0};

    if ((fd_req = open(REQ_PIPE, O_WRONLY)) == -1) 
    {
        perror("Server offline. Cannot open request pipe");
        exit(1);
    }
    if (write(fd_req, command, strlen(command)) == -1) 
    {
        perror("Write to server failed");
        close(fd_req);
        exit(1);
    }
    close(fd_req);

    // Reading the task ID from the orchestrator
    if (strncmp(command, "execute", 7) == 0) 
    //task ID only for execute commands
    {
        if ((fd_resp = open(RESP_PIPE, O_RDONLY)) == -1) 
        {
            perror("Server offline - cannot open response pipe");
            exit(1);
        }
        if (read(fd_resp, task_id, sizeof(task_id) - 1) > 0) 
        {
            sprintf(output_message, "TASK %s Received\n", task_id);
            write(STDOUT_FILENO, output_message, strlen(output_message));
        } 
        else 
        {
            perror("Failed to read task ID from server");
            close(fd_resp);
            exit(1);
        }
        close(fd_resp);
    }
}

void read_status() 
{
    int fd_resp = open(RESP_PIPE, O_RDONLY);

    if (fd_resp == -1) 
    {
        perror("Server offline on read - cannot open response pipe");
        exit(1);
    }

    char response[1024]; //buffer auxiliar à leitura do status
    int total_read = 0;
    // Ler a resposta até encontrar "END"
    while (1) 
    {
        ssize_t count = read(fd_resp, response + total_read, sizeof(response) - 1 - total_read);
        if (count > 0) 
        {
            total_read += count;
            response[total_read] = '\0';  // Garantir terminação da string
            if (strstr(response, "END")) 
            {
                *strstr(response, "END") = '\0';
                break;
            }
        } 
        else if (count == 0) 
        {
            perror("Pipe foi fechada");
            break;
        } 
        else 
        {
            perror("Read from server failed");
            break;
        }
    }
    write(STDOUT_FILENO, response, strlen(response));
    close(fd_resp);
}



int main(int argc, char* argv[]) 
{
    if (argc < 2) 
    {
        write(STDOUT_FILENO, "Usage: ./client <command> [<args>...]\n", 37); //debugging
        return 1;
    }

    char full_command[1024] = {0};

    if (strcmp(argv[1], "shutdown") == 0) 
    {
        send_request(argv[1]);
    } 

    else if (strcmp(argv[1], "execute") == 0) 
    {
        if (argc < 5) 
        {
            write(STDOUT_FILENO, "Insufficient arguments for execute.\n", 36);
            return 1;
        }
        
        if (!is_integer(argv[2])) 
        {
            char error_message[128];
            sprintf(error_message, "Invalid duration!: %s\n", argv[2]);
            write(STDOUT_FILENO, error_message, strlen(error_message));
            return 1;
        }

        // Construct the full command to send to the server
        strcat(full_command, "execute ");
        strcat(full_command, argv[3]);
        strcat(full_command, " ");
        strcat(full_command, argv[2]);
        strcat(full_command, " ");

        for (int i = 4; i < argc; i++) 
        {
            strcat(full_command, argv[i]);
            strcat(full_command, " ");
        }

        send_request(full_command); //send exec request

    } 

    else if (strcmp(argv[1], "status") == 0) 
    {
        send_request("status");
        read_status();
    } 
    else 
    {
        write(STDOUT_FILENO, "Unknown command.\n", 17);
        return 1;
    }

    return 0;
}
