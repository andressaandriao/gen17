/*
 * host.c
 *
 *  Created on: 05/06/2015
 * 
 */

//OBS: SERA NECESSARIA UMA VARIAVEL PARA PASSAR DO MENU PARA AS THREADS 
//A INFORMACAO DE QUAL IP OU USUARIO E. DA PARA SER DA MESMA MANEIRA QUE AS VARIAVEIS QUE O MENU ATIVA.

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>
#include <semaphore.h>

#define PORTA 22000
#define MAXHOSTS 50

//Variaveis globais//////////

typedef struct pcdata {
		char hostip[16];
		char hostname[50];
		int clientserver;	//0 = ele e cliente. 1 = ele e servidor.
	} hostdata;

hostdata hostslist[MAXHOSTS];//fazer uma lista encadeada?

int numdecontatos;    //Guarda o numero de contatos adicionados ate o momento. Tanto como cliente quanto servidor.

int client_add;       //Variavel para passar a informacao do menu para thread cliente sobre adicionar contato
int client_send;      //variavel para passar informacao do menu para thread cliente para enviar mensagem
int server_send;      //variavel para passar informacao do menu para thread servidor para enviar mensagem
int prog_end;         //verifica se e o fim do programa nas threads cliente e servidor


sem_t 	sem_client,			//Semaforo para esperar o cliente
		sem_server, 		//Semaforo para esperar o servidor
		sem_hostslist;		//Semaforo para proteger hostslist (regiao critica)


//fim variaveris globais//////

/*******************************************************************************
 *	NOME:		sendmessage
 *	FUNÇÃO:		Enviar mensagem a partir do socket.
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

	printf("Digite a mensagem: ");
	fgets(sendline, 100, stdin); /*stdin = 0 , for standard input */
	__fpurge(stdin);

	write(sockfd, sendline, strlen(sendline)+1);
}

/*******************************************************************************
 *	NOME:		clientfunc
 *	FUNÇÃO:		Thread do cliente. Fara todas as operacoes que estao ligadas ao
 *				cliente.
 *
 *	RETORNO:	void
 *******************************************************************************/
void clientfunc(){

    char pcip[16];	//16 pq 4*3(max numeros) + 3(pontos) + 1(\n)

    int sockfd;
    char sendline[100];
    struct sockaddr_in servaddr;

    //Continua no loop enquanto menu nao avisar que o programa chegou ao fim atraves da variavel global
    while(prog_end != 1){
    	/* As variaveis sao regioes criticas, porem nao havera problema de conflito.
    	 * Quando o cliente mexe na regiao critica, o menu, que e o unico outro
    	 * que mexe com ela, estara dormindo. Como o cliente esta em loop
    	 * tambem nao ha problema caso ela seja atualizada antes da hora. Ele ira funcionar
    	 * De qualquer forma em algum momento. Nao ha deadlock.*/
    	 
    	//Se na thread menu o usuario deseja adicionar novo contato
    	if(client_add == 1){
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

			  bzero(&servaddr,sizeof(servaddr));
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
				  strtok(hostslist[numdecontatos].hostname, "\n"); //Tira o \n no final
				  __fpurge(stdin);
				  hostslist[numdecontatos].clientserver = 0;
				  printf("Dados salvos com sucesso\n\n");
				  numdecontatos++;
			  }
			  //Variavel global volta a ser 0. Somente o menu pode muda-la para 1 e fazer com que 
			  //o cliente adicione novo contato.
			  client_add = 0;
			  //Acorda a thread menu
			  sem_post(&sem_client);
    	}
    	
    	//Menu avisou pela variavel global que o cliente deve enviar uma mensagem
    	if(client_send == 1){
			  sendmessage(sendline, sockfd);
			  
			  //Variavel global volta a ser 0. Somente o menu pode muda-la para 1 e fazer com que 
    		client_send = 0;
    		
    		//Acorda a thread menu
    		sem_post(&sem_client);
    	}
    }

	close(sockfd);
}

/*******************************************************************************
 *	NOME:		serverfunc
 *	FUNÇÃO:		Thread do servidor. Realizara todas as funcoes ligadas ao servidor.
 *
 *	RETORNO:	void
 *******************************************************************************/
void serverfunc(){

  char str[100];
  char ip[16];
  int listen_fd, comm_fd;

  struct sockaddr_in servaddr, clientaddr;

  listen_fd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htons(INADDR_ANY);
	servaddr.sin_port = htons(PORTA);

	bind(listen_fd, (struct sockaddr*) &servaddr, sizeof(servaddr));

	listen(listen_fd, 10);

	int clientaddrlen = sizeof(struct sockaddr_in);
	comm_fd = accept(listen_fd, (struct sockaddr*) &clientaddr, (socklen_t*)&clientaddrlen);
	if(comm_fd < 0){
		printf("Erro ao aceitar conexao no servidor");
	}

	 while(prog_end != 1)
	{

		bzero(str, 100);

		if(read(comm_fd, str, 100))
		{
			printf("%s sent a message: %s", inet_ntoa(clientaddr.sin_addr), str);
			//write(comm_fd, str, strlen(str)+1); //no more echoing
		}

	}
    close(listen_fd);
}

/*******************************************************************************
 *	NOME:		send_message
 *	FUNÇÃO:		Enviar mensagem. Ela verifica na lista de hosts se o usuario para
 *				o qual se deseja enviar a mensagem e seu cliente ou servidor.
 *				A partir disso, manda a informacao para thread cliente ou servidor.
 *
 *	RETORNO:	void
 *******************************************************************************/
int send_message(){
	int i, erro;
	char option;
	char stringAux[50];
	printf("Para quem gostaria de mandar a mensagem?\n1 - Endereco IPv4 do destinatario\n2 - Nome do destinatario\n");
	erro = 1;
	__fpurge(stdin);
	option = getchar();
	if(option == '1'){
		printf("Digite o endereco IPv4\n");
		__fpurge(stdin);
		fgets(stringAux, 16, stdin);
		for(i = 0; i < numdecontatos; i++){
			if(strcmp(hostslist[i].hostip, stringAux) == 0){
				erro = 0;
				break;
			}
		}
	}
	else if(option == '2'){
		printf("Digite o nome\n");
		__fpurge(stdin);
		fgets(stringAux, 50, stdin);
		strtok(stringAux, "\n");
		for(i = 0; i < numdecontatos; i++){
			if(strcmp(hostslist[i].hostname, stringAux) == 0){
				erro = 0;
				break;
			}
		}
	}
	//Erro = 0 indica que o IP ou usuario desejado foi encontrado
	if (erro == 0){
	  //Usuario e servidor na determinada conexao
		if(hostslist[i].clientserver == 1){
			return 1;
		}
		//Usuario e cliente na determinada  conexao
		else{
			return 0;
		}
	}
	else{
		printf("Contato nao encontrado");
		return 2;
	}
}

/*******************************************************************************
 *	NOME:		list_contacts
 *	FUNÇÃO:		Lista os contatos
 *
 *	RETORNO:	void
 *******************************************************************************/
void list_contacts(){
	int i;
	printf("Nome\t\tIP\n");
	for(i=0; i<numdecontatos; i++)
	{
		printf("%s", hostslist[i].hostname);
		printf("\t\t%s\n", hostslist[i].hostip);
	}
	printf("\n");
}

/*******************************************************************************
 *	NOME:		clientfunc
 *	FUNÇÃO:		Thread do cliente. Fara todas as operacoes que estao ligadas ao
 *				cliente.
 *
 *	RETORNO:	void
 *******************************************************************************/
void menu_handle(){
	//Variaveis para o menu
	int tempchoice;
	char choice[2];

	int aux;

    do{

		__fpurge(stdin);
		printf("Deseja:\n1-Inserir Contato\n2-Listar Contatos\n3-Excluir Contato\n4-Enviar Mensagem\n5-Mensagem em Grupo\n6-Sair\n");
		choice[0] = getchar();
		__fpurge(stdin);
		choice[1] = '\0';//manter a semantica de atoi (precisa de \0)
		tempchoice = atoi(choice);
		__fpurge(stdin);

		switch (tempchoice)
		{
			case 1:
				client_add = 1;
				//Dorme enquanto espera a thread cliente executar.
				sem_wait(&sem_client);
				break;

			case 2:
				list_contacts();
				break;

			case 3:
				printf("Digite o nome ou IP do contato que deseja excluir");
				break;

			case 4:
				aux = send_message();
				//Se for cliente, dorme esperando o cliente terminar.
				if (aux == 0){
					client_send = 1;
					sem_wait(&sem_client);
				}
				//Se for servidor, dorme esperando o servidor terminar.
				else{
					server_send = 1;
					sem_wait(&sem_server);
				}
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

    prog_end = 1;
}

/*******************************************************************************
 *	NOME:		init_semaphores
 *	FUNÇÃO:		Inicia os semaforos
 *
 *	RETORNO:	void
 *******************************************************************************/
void init_semaphores() {
	sem_init(&sem_client, 0, 0);
	sem_init(&sem_server, 0, 0);
	sem_init(&sem_hostslist, 0, 0);
}

/*******************************************************************************
 *	NOME:		init_threads
 *	FUNÇÃO:		Inicia as threads de servidor e cliente e menu
 *
 *			Tipo					Descrição
 *     		pthread_t*				Thread servidor declarada na main
 *     		pthread_t*				Thread cliente declarada na main
 *
 *	RETORNO:	int	(0 se threads foram criadas com sucesso, 1 se nao foram)
 *******************************************************************************/
int init_threads(pthread_t *serverthread, pthread_t *clientthread, pthread_t *menuthread){

	if(pthread_create(menuthread, 0, (void*)menu_handle, NULL) != 0){
		printf("Erro na criacao da thread menu.\n");
		return 1;
	}
	if(pthread_create(serverthread, 0, (void*)serverfunc, NULL) != 0){
		printf("Erro na criacao da thread servidor.\n");
		return 1;
	}

	if(pthread_create(clientthread, 0, (void*)clientfunc, NULL) != 0){
		printf("Erro na criacao da thread servidor.\n");
		return 1;
	}

	return 0;
}

/*******************************************************************************
 *	NOME:		main
 *	FUNÇÃO:		Chama as funcoes que iniciam as threads
 *
 *			Tipo					Descrição
 *     		pthread_t*				Thread servidor declarada na main
 *     		pthread_t*				Thread cliente declarada na main
 *
 *	RETORNO:	int	(0 se threads foram criadas com sucesso, 1 se nao foram)
 *******************************************************************************/
int main(int argc,char **argv){
	int erro = 0;
	prog_end = 0;
	client_add = 0;
	client_send = 0;
	server_send = 0;
	numdecontatos = 0;

	pthread_t serverthread, clientthread, menuthread;

	init_semaphores();
	erro = init_threads(&serverthread, &clientthread, &menuthread);
	if (erro == 1){	//Significa que as threads nao foram criadas. Logo o programa nao pode prosseguir.
		return 1;
	}
	else{
		pthread_join(menuthread, NULL);
	    pthread_join(clientthread, NULL);
	    pthread_join(serverthread, NULL);
		return 0;
	}
}
