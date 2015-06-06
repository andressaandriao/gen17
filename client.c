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
	
	printf("Digite a mensagem: ");
	fgets(sendline, 100, stdin); /*stdin = 0 , for standard input */
	
	write(sockfd, sendline, strlen(sendline)+1);
}


//Variaveis globais//////////

typedef struct pcdata {
		char hostip[16];
		char hostname[50];
	} hostdata;
	
	hostdata hostslist[50];//fazer uma lista encadeada?

int numdecontatos = 0;

//fim variaveris globais//////
 
 
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
		

    int tempchoice;
    char choice[2];
    char pcip[16];//16 pq 4*3(max numeros) + 3(pontos) + 1(\n)
    int i;
 
    
    int sockfd;
    char sendline[100];
    struct sockaddr_in servaddr;
    
    do{
		
		__fpurge(stdin);
		printf("Deseja:\n1-Inserir Contato\n2-Listar Contatos\n3-Excluir Contato\n4-Enviar Mensagem\n5-Mensagem em Grupo\n6-Sair\n");
		choice[0] = getchar();
		choice[1] = '\0';//manter a semantica de atoi (precisa de \0)
		tempchoice = atoi(choice);
		//printf("%c ", choice[0]); //prints para teste
		//printf("%d\n", tempchoice);
		__fpurge(stdin);
		
		switch (tempchoice)
		{
			case 1:
				printf("Digite o ip do contato que deseja inserir: ");
				fgets(hostslist[numdecontatos].hostip, 16, stdin);
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
				}
				else
				{
					printf("Digite o apelido para o host de IP %s", hostslist[numdecontatos].hostip);
					__fpurge(stdin);
					fgets(hostslist[numdecontatos].hostname, 50, stdin);
					__fpurge(stdin);
					printf("Dados salvos com sucesso\n\n");
					numdecontatos++;
				}
				break;
				
			case 2:
				printf("Nome\t\tIP\n");
				for(i=0; i<numdecontatos; i++)
				{
					int j=0;
					while(hostslist[i].hostname[j] != '\n')
					{//este while impede a impressao de um \n apenas para fins esteticos de apresentacao
						printf("%c", hostslist[i].hostname[j]);
						j++;
					}
					printf("\t\t%s\n", hostslist[i].hostip);
				}
				printf("\n");
				break;
				
			case 3:
				printf("Digite o nome ou IP do contato que deseja excluir");
				break;
				
			case 4:
				printf("Para quem gostaria de mandar a mensagem?\nDigite o nome ou endereco IPv4 do destinatario\n");
				sendmessage(sendline, sockfd);
				break;
				
			case 5:
				break;
				
			case 6:
				printf("Adeus!\n");
				break;
			
			default:
				printf("Escolha invalida\n\n");
				break;
		}
		
	}while(tempchoice!=6);
	
	close(sockfd);
	
	return 0;
 
}
