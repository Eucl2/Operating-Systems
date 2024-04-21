#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#define MAX_LINE_SIZE 1000


ssize_t readln (int fd, char* line, size_t size)
{

	int read_bytes = 0, pos = 0;
	
	while(pos < size && read(fd, line+pos, 1)>0){
		read_bytes++;
		if(line[pos]=='\n'){
			break;
		}							
		pos++;
	}
	return read_bytes;
}

void status()
{

}

int main(){

    int res = mkfifo("fifo", 0666);

    if(res == -1){
        perror("mkfifo");
    }

    int fd_log = open("log.txt", O_CREAT| O_WRONLY| O_TRUNC);
    if(fd_log==-1){
        perror("fd_log");
    }

    while(1){
        int fd_fifo = open("fifo", O_RDONLY);
        int fd_fifo_write = open("fifo", O_WRONLY);//assim o feicheiro nunca fecha para a escrita, vai permanecer sempre aberto, e o servidor nunca morre
        //se se fizer isto tem de se abrir o leitor primeiro, e só dpeois o de escrita, senão, dá deadlock

        if(fd_fifo < 0){
            perror("open");
        }else{
            printf("opened fifo for reading..\n");

        }
        int bytes_read=0;
        char buffer[MAX_LINE_SIZE];

        while((bytes_read = read(fd_fifo, &buffer, MAX_LINE_SIZE))>0){
            buffer[bytes_read] = '\0';
            int bytes_write = write(fd_log, &buffer, bytes_read+1);//tem de ser +1 por causa de se ter adicionado o '\0'
        }

        close(fd_fifo);
        unlink("fifo");
    }
    return 0;
}





