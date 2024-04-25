#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h> //this one imp
#include <stdio.h>
#include <fcntl.h>
#include <string.h>


void send_request(char arg[])
{
    int fd_fifo;
	if((fd_fifo = open("fifo", O_WRONLY))==-1) perror("Server offline");

    // falta mandar para o servidor os argumentos e o cliente receber um PID para identificar
    // comunica esse pid ao utilizador
    //o programa cliente termina e o utilizador pode efetuar outras operaçoes
    //mesmo sem terminadas

}



int main(int argc, char* argv[])
{

    int fd_fifo, fd_fifostatus;

    // argumentos necessarios 
    if(argc==1)
    {
        printf("Sem argumentos a executar pelo cliente.\n"); exit(1);
    }
    if(strcmp(argv[1],"execute")==0)
    {
        if(argc==2)
        {
            printf("Sem tempo a executar.\n"); exit(1);
        }
        if(argv[2])// verifica se o time existe. Adicionar verificação se é int
        {
			if(argc==3) 
            {
                printf("Sem argumento a executar.\n"); exit(1);
            }
            if(strcmp(argv[3],"-u")==0)
            {
               if(argc==4)
               {
                printf("Sem programa a executar.\n"); exit(1);
               }
                //tem tudo , passa a executar aka inicio do fifo e conecçao ao servidor
			    send_request(argv[4]);// alterar nome 
			} 
            if(strcmp(argv[3],"-p")==0)
            {
                if(argc==4)
               {
                printf("Sem programa a executar.\n"); exit(1);
               }
               char* progs[20];
			   int i=0;
			   char* token = strtok(argv[4],"|");
               while(token != NULL)
                {
				    progs[i] = token;
				    token = strtok(NULL,"|");
				    i++;
			    }
                //tem tudo , passa a executar
			    multiexec(i,progs);// alterar nome 
			}
        }
    }
    if(strcmp(argv[1],"status")==0)
    {
        if((fd_fifo = open("fifo", O_WRONLY)) == -1) perror("Server offline");
		
		int write_bytes = write(fd_fifo,argv[1],strlen(argv[1]));
		close(fd_fifo);
		
		sleep(1);



    }
			





    int fd_fifo = open("fifo", O_WRONLY|O_CREAT);

    if(fd_fifo < 0)
    {
        perror("open");
    }
    else
    {
        printf("opened fifo for writing...\n");
    }

    int bytes_read=0;
    //char buffer[MAX_LINE_SIZE];
    //while((bytes_read = read(0, &buffer, MAX_LINE_SIZE))){
    int bytes_written = write(fd_fifo, argv[1], strlen(argv[1]));

    if (bytes_written ==1)
    {
        perror ("write");
    }
    else
    {
        printf("written to fifo %d bytes", bytes_written);
    }

    close(fd_fifo);
    unlink("fifo");

    return 0;
}