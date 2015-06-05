#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
void serverfunc()//int main(int argc,char **argv) //transformar aqui em main para testar sem threads
{
	
    char str[100];
    int listen_fd, comm_fd;
 
    struct sockaddr_in servaddr, senderaddr;
 
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
 
    bzero( &servaddr, sizeof(servaddr));
 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(22000);
 
    bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr));
 
    listen(listen_fd, 10);
 
	int senderaddrlen = sizeof(struct sockaddr_in);
    comm_fd = accept(listen_fd, (struct sockaddr*) &senderaddr, (socklen_t*)&senderaddrlen );
	//printf("Greetings from server\n");
    while(1)
    {
 
        bzero(str, 100);
 
        if( read(comm_fd, str, 100) )
        {
			printf("%s sent a message: %s", inet_ntoa( senderaddr.sin_addr ), str);
			//write(comm_fd, str, strlen(str)+1); //no more echoing
        }
 
    }
    
    close(listen_fd);
    
    //return 0;
}
