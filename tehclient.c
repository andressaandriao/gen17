#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>
#include "common_variables.h"

#define PORTA 22000

/*******************************************************************************
 *	NOME:		sendmessage
 *	FUNÇÃO:		Envia mensagem pelo socket do cliente
 *
 *			Tipo					Descrição
 *     		char* sendline
 *     		int   sockfd
 *
 *	RETORNO:	void
 *******************************************************************************/
void sendmessage(char *sendline, int sockfd)
{
	bzero(sendline, 100);
	//bzero( recvline, 100);
	
	printf("Digite a mensagem: ");

	//POSSIVEL TRATAMENTO PARA ENVIAR MENSAGENS MAIS COMPRIDAS!
	fgets(sendline, 100, stdin); /*stdin = 0 , for standard input */
	
	write(sockfd, sendline, strlen(sendline)+1);
	//read(sockfd,recvline,100); //no more echoing
	//printf("%s",recvline);
}

/*******************************************************************************
 *	NOME:		clientfunc
 *	FUNÇÃO:		Thread cliente. Contem o menu principal tambem, onde o usuario
 *				escolhe as funcoes de adicionar contato, enviar mensagens e sair.
 *
 *			Tipo					Descrição
 *     		void *thread_id			Identificacao da thread
 *
 *	RETORNO:	void
 *******************************************************************************/
void *clientfunc(void *thread_id){

	/*typedef struct pcdata {
		char hostip[16];
		char hostname[50];
	}hostdata;

	hostdata hostslist;
	*/

	//Variaveis para o menu
    int tempchoice;
    char choice[2];

	int numdecontatos = 0;
    char pcip[16];				//16 pq 4*3(max numeros) + 3(pontos) + 1(\n)
 
    
    int sockfd;
    char sendline[100];
    //char recvline[100];
    struct sockaddr_in servaddr;

    do{

		__fpurge(stdin);
		printf("Deseja:\n1-Inserir contato\n2-mandar mensagem\n3-sair\n\n");
		choice[0] = getchar();
		__fpurge(stdin);
		choice[1] = '\0';//manter a semantica de atoi (precisa de \0)
		tempchoice = atoi(choice);
		//printf("%c ", choice[0]); //prints para teste
		//printf("%d\n", tempchoice);
		__fpurge(stdin);
		
		switch (tempchoice)
		{
			case 1:
				printf("Digite o ip do contato que deseja inserir: ");
				fgets(hostslist.hostip, 16, stdin);
				__fpurge(stdin);
				sockfd = socket(AF_INET, SOCK_STREAM, 0);
				//printf("\n%s\n", hostslist.hostip);
				
				if(sockfd == -1)//erro
				{
					perror("Socket dun goofed");
					exit(1);//fazer tratamento de erro melhor
				}
					
				bzero(&servaddr,sizeof(servaddr));
				servaddr.sin_family=AF_INET;
				servaddr.sin_port=htons(PORTA);
 
				inet_pton(AF_INET, pcip, &(servaddr.sin_addr));//127.0.0.1 (ip targeting self)
 
				if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
				{
					perror("Connect dun goofed");
					close(sockfd);//closing dedicated socket
					//exit(1);
				}
				else
				{
					printf("Digite o apelido para o host de IP %s", hostslist.hostip);
					__fpurge(stdin);
					fgets(hostslist.hostname, 50, stdin);
					__fpurge(stdin);
					printf("Dados salvos com sucesso\n\n");
				}
				break;
				
			case 2:
				sendmessage(sendline, sockfd);
				break;
			
			case 3:
				printf("Adeus!\n");
				break;
			
			default:
				printf("Escolha invalida\n\n");
				break;
		}

	}while(tempchoice!=3);
	
	close(sockfd);
}
