#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "defs.h"
#include "vector.h"

//FIFO criado pelo servidor
//Cliente pode receber um sigpipe (concorrÃªncia!)

//tem de se procurar o numero de ocorrencias de um dado nÂº pedido pelo cliente

//open read
//wjile(read > 0)
//close

#define MAX_TASKS 10
#define MAX_LINE_SIZE 1000

typedef struct task 
{
    int id;
    char command[MAX_CMD_LEN];
    int duration; // Tempo estimado de execução em milissegundos
    int status; // 0: Em espera, 1: Em execução, 2: Terminada
    pthread_t thread_id;
} task_t;

task_t tasks[MAX_TASKS];

int num_tasks = 0;
int parallel_tasks = 4; // Número máximo de tarefas em execução em paralelo
int sched_policy = 0; // Identificador da política de escalonamento (FCFS, SJF, etc.)

void *execute_task(void *arg) 
{
    task_t *task = (task_t *)arg;

    // Executar o comando da tarefa
    char *command = task->command;
    system(command);

    // Registrar o tempo de execução da tarefa
    task->status = 2;
    // ...

    return NULL;
}

void handle_client_request() 
{
    char buffer[MAX_CMD_LEN + 1];

    // Ler comando e argumentos do cliente
    int bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
    if (bytes_read < 0) 
    {
        perror("read");
        return;
    }

    char *command = strtok(buffer, " ");
    if (!command) 
    {
        return;
    }

    char *arguments = strtok(NULL, "\n");
    if (!arguments) 
    {
        return;
    }

    // Processar o comando
    if (strcmp(command, "execute") == 0) 
    {
        // ...
    } 
    else if (strcmp(command, "status") == 0) 
    {
        // ...
    } 
    else
    {
        fprintf(stderr, "Comando inválido: %s\n", command);
    }
}


int main (int argc, char * argv[]){

    if (argc < 4)
    {
        fprintf(stderr, "Uso correto: %s <output_folder> <parallel_tasks> <sched_policy>\n", argv[0]);
        exit(1);
    }

	int res = mkfifo(SERVER, 0666);
	
	if(res == -1)
    {
		perror("mkfifo");	
	}
	
	while(1)
    {		
		
		int fds;
		Msg m;
		
		//open server for reading
		if((fds_read = open(SERVER, O_RDONLY))== -1)
        {
			perror("open server fifo");
			return -1;
		}
        else
        {
			printf("[debug] oppened server for reading\n",SERVER )
		}
		
		//abrir aqui tambem para escrita para garantir que nao acontece end of file
		//ao haver os dois cenas abertos, acho que mudava mias merdas no codigo do que quando estava so a leitura aberta, but i odnt remember what cause she went to fast :((
		if((fds_write = open(SERVER, O_RDONLY))== -1)
        {
			perror("open server fifo");
			return -1;
		}
        else
        {
			printf("[debug] oppened server for reading\n",SERVER )
		}
		
		
		
		int bytes_read;
		while((bytes_read=read(fds, &m. sizeof(m)))>0)
        {
			if(bytes_read == sizeof(m))
            {
			
			//falta cenas
			}
		}

		
		char fifoc_name[30];
		snprintf(fifoc_name, CLIENT "%d", m.pid);
		int fdc;
		 //
		if((fdc=open(fifoc_name, O_WRONLY))==-1)
        {
			perror("");
			continue;
		}
        else
        {
			printf("0ppend %s for writting", fifoc_name);
		}
		
		write(fdc, &m, sizeof(m))
		close(fdc);
		
	}
	close(fds_read);
	
	unlink(SERVER);
	
	return 0;
}


