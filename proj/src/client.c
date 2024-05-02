#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define MAX_ARGS 20 //to review
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
    if (command != NULL) 
    {
        printf("Sending [ %s ] request to orchestrator... \n", command); //debugging
    } 
    else 
    {
        printf("Error: Command is NULL\n"); //debugging
    }
    int fd_req, fd_resp;
    char task_id[100] = {0};

    if ((fd_req = open(REQ_PIPE, O_WRONLY)) == -1) 
    {
        perror("Server offline - cannot open request pipe");
        exit(1);
    }
    // strcat(command, "\n"); //trying to avoid errors reading the pipe on orchestrator
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
            printf("TASK %s Received\n", task_id);
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
    printf("Reading status from orchestrator...\n\n"); //debugging
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
                break;  // Sair do loop quando "END" for recebido
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
    printf("%s", response);  // Mostrar resposta => podemos usar printf neste caso?
    close(fd_resp);
}



int main(int argc, char* argv[]) 
{
    if (argc < 2) 
    {
        fprintf(stderr, "Usage: ./client <command> [<args>...]\n"); //DEBUG - FPRINTF isnt allowed, i think - keep for debugging only
        fprintf(stderr, "Commands: execute -u|-p <time> <command>\n");
        fprintf(stderr, "          status\n");
        fprintf(stderr, "Provided command: %s\n", argv[1]);
        return 1;
    }

    char full_command[1024] = {0};

    if (strcmp(argv[1], "shutdown") == 0) 
    {
        printf ("Detetado pedido de shutdown. A enviar para o orchestrator...\n"); //debug
        send_request(argv[1]);
    }

    else if (strcmp(argv[1], "execute") == 0) 
    {
        if (argc < 5) 
        {
            fprintf(stderr, "Insufficient arguments for execute.\n");
            return 1;
        }

        // Check if argv[2] (aka duration) is a valid integer - Useless??
        
        if (!is_integer(argv[2])) 
        {
            fprintf(stderr, "Invalid duration!: %s\n", argv[2]);
            return 1;
        }

        // Construct the full command to send to the server
        strcat(full_command, "execute ");
        strcat(full_command, argv[3]); // -u or -p
        strcat(full_command, " ");
        strcat(full_command, argv[2]); // duration
        strcat(full_command, " ");

        // Concatenate all program and args
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
        fprintf(stderr, "Unknown command.\n");
        return 1;
    }

    return 0;
}
