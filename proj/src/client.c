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
    int fd_req;
    char task_id[100] = {0};

    if ((fd_req = open(REQ_PIPE, O_WRONLY)) == -1) 
    {
        perror("Server offline - cannot open request pipe");
        exit(1);
    }

    if (write(fd_req, command, strlen(command)) == -1) 
    {
        perror("Write to server failed");
        close(fd_req);
        exit(1);
    }
    close(fd_req);

    //missing: reading the id da tarefa vinda do orchestrator
}

void read_status() 
{
    printf("Reading status from orchestrator...\n"); //debuging
    int fd_resp = open(RESP_PIPE, O_RDONLY);
    
    if (fd_resp == -1) 
    {
        perror("Server offline on read - cannot open response pipe");
        exit(1);
    }

    char response[1024];
    // Ler a resposta até encontrar "END"
    while (1) 
    {
        printf("Starting to read after opening pipe...\n"); //debuging
        ssize_t count = read(fd_resp, response, sizeof(response) - 1);
        if (count > 0) 
        {
            printf("counting...\n"); //debugging
            response[count] = '\0';  // Garantir terminação da string
            if (strstr(response, "END")) //é permitido usar strstr? 
            {
                break;  // Sair do loop quando "END" for recebido
            }
            printf("response:\n%s", response);  // Mostrar resposta
        } 
        else if (count == 0) 
        {
            printf("pipe was closed :( )"); //debugging
            break;
        } 
        else 
        {
            // Erro de leitura
            perror("Read from server failed");
            break;
        }
    }
    close(fd_resp);
}


int main(int argc, char* argv[]) 
{
    if (argc < 3 && strcmp(argv[1], "status") != 0) 
    {
        fprintf(stderr, "Usage: ./client <command> [<args>...]\n"); //DEBUG - FPRINTF isnt allowed, i think - keep for debugging only
        fprintf(stderr, "Commands: execute -u|-p <time> <command>\n");
        fprintf(stderr, "          status\n");
        fprintf(stderr, "Provided command: %s\n", argv[1]);
        return 1;
    }

    char full_command[1024] = {0}; //review
    if (strcmp(argv[1], "execute") == 0) 
    {
        if (argc < 5) 
        {
            fprintf(stderr, "Insufficient arguments for execute.\n");
            return 1;
        }

        // Check if argv[2] (aka duration) is a valid integer - Useless?
        for (char* p = argv[2]; *p; p++) {
            if (!isdigit(*p)) {
                fprintf(stderr, "Invalid duration: %s\n", argv[2]);
                return 1;
            }
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
