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
        printf("Hey from client! Sending: %s\n", command); //debugging
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

    if (write(fd_req, command, strlen(command)) == -1) 
    {
        perror("Write to server failed");
        close(fd_req);
        exit(1);
    }
    close(fd_req);

    if ((fd_resp = open(RESP_PIPE, O_RDONLY)) == -1) 
    {
        perror("Server offline on read - cannot open response pipe");
        exit(1);
    }

    // Read the task ID or status from the orchestrator
    if (read(fd_resp, task_id, sizeof(task_id)) <= 0) 
    {
        perror("Read from server failed");
    } 
    else 
    {
        printf("Response from server: %s\n", task_id); //can we use printf here? if not, how to show to user the response(s)?
    }
    close(fd_resp);
}

int main(int argc, char* argv[]) 
{
    if (argc < 3 && strcmp(argv[1], "status") != 0) 
    {
        fprintf(stderr, "Usage: ./client <command> [<args>...]\n"); //DEBUG - FPRINTF isnt allowed - keep for debugging only
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
    } 
    else if (strcmp(argv[1], "status") == 0) 
    {
        strcpy(full_command, "status");
    } 
    else 
    {
        fprintf(stderr, "Unknown command.\n");
        return 1;
    }

    send_request(full_command);
    return 0;
}
