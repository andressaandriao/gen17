#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <pthread.h> 
#include "echoserver.c"//comentar aqui para retirar threads

#define PORTA 22000

void sendmessage(char *sendline, int sockfd)
{
	bzero( sendline, 100);
	//bzero( recvline, 100);
	
	printf("Digite a mensagem: ");
	fgets(sendline, 100, stdin); /*stdin = 0 , for standard input */
	
	write(sockfd, sendline, strlen(sendline)+1);
	//read(sockfd,recvline,100); //no more echoing
	//printf("%s",recvline);
}
 
int main(int argc,char **argv)
{
	
	/*criar thread*/
	pthread_t serverthread;
	
	if( pthread_create(&serverthread, NULL, (void *)serverfunc, NULL) != 0)
	{
		printf("Erro em criar a thread de servidor.\nSaindo do programa.");
		exit(1);
	}
	else
		printf("Thread created\n\n");
	/*criar thread, fim*/
		
	
	typedef struct pcdata {
		char hostip[16];
		char hostname[50];
	} hostdata;
	
	hostdata hostslist;//fazer uma lista encadeada

    int tempchoice, numdecontatos = 0;
    char choice[2];
    char pcip[16];//16 pq 4*3(max numeros) + 3(pontos) + 1(\n)
 
    
    int sockfd;
    char sendline[100];
    //char recvline[100];
    struct sockaddr_in servaddr;
    
    do{
		
		__fpurge(stdin);
		printf("Deseja:\n1-Inserir contato\n2-mandar mensagem\n3-sair\n\n");
		choice[1] = getchar();
		choice[2] = '\0';//manter a semantica de atoi (precisa de \0)
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
					
				bzero(&servaddr,sizeof servaddr);
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
	
	return 0;
 
}
