#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_variables.h"
#define PORTA 22000

 
/*******************************************************************************
 *	NOME:		serverfunc
 *	FUNÇÃO:		Thread servidor. Recebe as conexoes de outros clientes.
 *
 *			Tipo					Descrição
 *     		void *thread_id			Identificacao da thread
 *
 *	RETORNO:	void
 *******************************************************************************/
void *serverfunc(void *thread_id)
{

    char str[100];
    int listen_fd, comm_fd;
 
    struct sockaddr_in servaddr, senderaddr;
 
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1){
    	printf("Erro na criacao do socket!");
    }
 
    bzero(&servaddr, sizeof(servaddr));
 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(PORTA);
 
    bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr));
 
    listen(listen_fd, 10);
 
	int senderaddrlen = sizeof(struct sockaddr_in);
    comm_fd = accept(listen_fd, (struct sockaddr*) &senderaddr, (socklen_t*)&senderaddrlen );
	//printf("Greetings from server\n");

    /*
    //Quando recebe uma nova conexao, adiciona na lista de clientes.
    printf("Digite o apelido para o host de IP %s", clientdata.);
	__fpurge(stdin);
	fgets(clientdata.clientname, 50, stdin);
	__fpurge(stdin);
	printf("Dados salvos com sucesso\n\n");
     */
    while(1)
    {
 
        bzero(str, 100);
 
        if( read(comm_fd, str, 100) )
        {
			printf("%s sent a message: %s", inet_ntoa(senderaddr.sin_addr ), str);
			//write(comm_fd, str, strlen(str)+1); //no more echoing
        }
 
    }
    
    close(listen_fd);
    
}
