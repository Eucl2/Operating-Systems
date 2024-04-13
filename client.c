#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "defs.h"

int main (int argc, char * argv[])
{

	if (argc < 3) 
    {
		printf("Missing argument.\n");
		_exit(1);
	}
	
	Msg m;
	m.pid = ??//pid maybe idk
	
	char fifoc_name[30];
	
    //crirar os fifos dos clientes ( a ana disse )
	sprintf(fifoc_name, CLIENT, "%d", m.pid);
	
	if(mkfifo(fifoc_name, 0666) == -1)
    {
		peeror("mkfifo client");
		return -1;
	}
	
    // abrir o fifo que foi criado no servidor 
	int fds = open(SERVER, O_WRONLY);
	
	if(fds == -1)
    {
		perror("open fifo server");
		return -1;
	}
    else
    {
		printf("oppened %s for writting", SERVER);
	}
	
	size_t len = strlen(comando) + 1 + strlen(argumentos) + 1;
    char *buffer = malloc(len);
    if (!buffer) 
    {
        perror("malloc");
        exit(1);
    }

    strcpy(buffer, comando);
    buffer[strlen(comando)] = ' ';
    strcpy(buffer + strlen(comando) + 1, argumentos);

	//send request
	if (write(fds, buffer, len) < 0) 
    {
        perror("write");
        exit(1);
    }
	
	free(buffer);
    close(fds);
	
	//open client fifo
	int fdc = open(fifoc_name, O_RDONLY);
	if(fdc == -1)
    {
		perror
		return -1;
	}
    else
    {
		printf("oppened %s for reading", fifoc_name)
	}
	
	//get reply
	if(read (fdc, &m, sizeof(m))<=0)
	//falta cenas
}

