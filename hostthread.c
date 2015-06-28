/*
 * host.c
 *
 *  Created on: 05/06/2015
 * 
 */

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

#define MAXHOSTS 20

//Variaveis globais//////////

typedef struct pcdata {
		char hostip[16];
		char hostname[50];
		int exist;				//Para falar se ele existe ou nao.
	} hostdata;

hostdata hostslist[MAXHOSTS+30]; //Taxa de erro e 30 (desconectar e conectar de novo).

typedef struct serveraddrhandler {
	
	int tempsock;
	char chat_ip[16];
} listenerthreadparameters;

int numdecontatos;    //Guarda o numero de contatos adicionados ate o momento. Tanto como cliente quanto servidor.

int client_add;       //Variavel para passar a informacao do menu para thread cliente sobre adicionar contato
int client_send;      //variavel para passar informacao do menu para thread cliente para enviar mensagem
int prog_end;         //verifica se e o fim do programa nas threads cliente e servidor
int contato;		  //Variavel para passar a informacao de para qual dos clientes do vetor e a mensagem.
int client_exclude;	  //Variavel para passar informacao do menu para thread cliente sobre excluir um contato.
int PORTA;			  //Variavel para a porta utilizada
FILE *chat_log;		  //Arquivo que guarda as mensagens enquanto o usuario nao voltou para o menu principal.
					  //Quando ele voltar para o menu principal, as mensagens sao printadas na tela.

sem_t 	sem_client;			//Semaforo para esperar o cliente
sem_t 	sem_file;			//Semaforo para a regiao critica do arquivo

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

	if(write(sockfd, sendline, strlen(sendline)+1) <= 0){
		perror("Mensagem nao enviada");
	}
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

    int sockfd[MAXHOSTS];
    char sendline[100];
    struct sockaddr_in servaddr;
    int i, added = 0;

    //Continua no loop enquanto menu nao avisar que o programa chegou ao fim atraves da variavel global
    while(prog_end != 1){
    	/* As variaveis sao regioes criticas, porem nao havera problema de conflito.
    	 * Quando o cliente mexe na regiao critica, o menu, que e o unico outro
    	 * que mexe com ela, estara dormindo. Como o cliente esta em loop
    	 * tambem nao ha problema caso ela seja atualizada antes da hora. Ele ira funcionar
    	 * De qualquer forma em algum momento. Nao ha deadlock.*/
    	 
    	//Se na thread menu o usuario deseja adicionar novo contato
    	if(client_add == 1){
			//Todas as informacoes estarao na regiao da struct. O servidor tbm as acessa
			//Por isso e necessario colocar um semaforo.
			printf("Digite o ip do contato que deseja inserir:\n ");
			fgets(pcip, 16, stdin);
			strtok(pcip, "\n");
			__fpurge(stdin);
			for(i = 0; i<numdecontatos; i++){
				if(strcmp(hostslist[i].hostip,pcip) == 0 && hostslist[i].exist == 1){
					added = 1;
				}
			}
			if(added == 1){
				printf("Contato ja existente!\n\n");
				client_add = 0;
				sem_post(&sem_client);
			}
			else{
				added = 0;
				strcpy(hostslist[numdecontatos].hostip, pcip);

				sockfd[numdecontatos] = socket(AF_INET, SOCK_STREAM, 0);

				if(sockfd[numdecontatos] == -1)//erro
				{
					perror("Socket dun goofed");
					client_add = 0;
					sem_post(&sem_client);
					exit(1); //fazer tratamento de erro melhor
				}

				bzero(&servaddr,sizeof(servaddr));
				servaddr.sin_family=AF_INET;
				servaddr.sin_port=htons(PORTA);

				inet_pton(AF_INET, pcip, &(servaddr.sin_addr));//127.0.0.1 (ip targeting self)

				if(connect(sockfd[numdecontatos], (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
				{
					perror("Connect dun goofed");
					close(sockfd);//closing dedicated socket
				}
				else
				{
					printf("Digite o apelido para o host de IP %s\n", hostslist[numdecontatos].hostip);
					__fpurge(stdin);
					fgets(hostslist[numdecontatos].hostname, 50, stdin);
					strtok(hostslist[numdecontatos].hostname, "\n"); //Tira o \n no final
					__fpurge(stdin);
					hostslist[numdecontatos].exist = 1;
					numdecontatos++;
					printf("Dados salvos com sucesso\n\n");
				}
				//Variavel global volta a ser 0. Somente o menu pode muda-la para 1 e fazer com que
				//o cliente adicione novo contato.
				client_add = 0;
				//Acorda a thread menu
				sem_post(&sem_client);
			}
    	}
    	//Menu avisou pela variavel global que o cliente deve enviar uma mensagem
    	if(client_send == 1){

			sendmessage(sendline, sockfd[contato]);

			//Variavel global volta a ser 0. Somente o menu pode muda-la para 1 e fazer com que
			client_send = 0;

			//Acorda a thread menu
			sem_post(&sem_client);
		}
    	//Caso for broadcast
    	if(client_send == 2){

    		bzero(sendline, 100);

    		printf("Digite a mensagem: ");
    		fgets(sendline, 100, stdin); /*stdin = 0 , for standard input */
    		__fpurge(stdin);

    		for(i = 0; i < numdecontatos; i++){
    			if(hostslist[i].exist == 1)
    				write(sockfd[i], sendline, strlen(sendline)+1);
    		}

    		//Variavel global volta a ser 0. Somente o menu pode muda-la para 1 e fazer com que
    		client_send = 0;

    		//Acorda a thread menu
    		sem_post(&sem_client);
    	}
    	if(client_exclude == 1){

    		close(sockfd[i]);

    		//Variavel global volta a ser 0. Somente o menu pode muda-la para 1 e fazer com que
    		client_exclude = 0;

    		printf("Contato excluido com sucesso\n");

    		//Acorda a thread menu
    		sem_post(&sem_client);
    	}
    }
   	close(sockfd);
}

/*******************************************************************************
 *	NOME:		serverlistener
 *	FUNÇÃO:		Thread que recebe as mensagens daqueles que tiverem conectados a
 * 				este pc como servidor
 *	RETORNO:	void
 *******************************************************************************/


void *serverlistener(void *conn_data)
{
	listenerthreadparameters connection_descriptor = *(listenerthreadparameters*)conn_data;
	char rcv_msg[1001];
	char fEmpty;
	int pos = 1+sizeof(int);
	bzero(rcv_msg, 1001);
	
	while(recv(connection_descriptor.tempsock, rcv_msg, 1001, 0) > 0 )
	{
		sem_wait(&sem_file);
		fseek(chat_log, 0, SEEK_SET);
		fEmpty = fgetc(chat_log);
		if(fEmpty == '0'){
			fwrite(&pos, sizeof(int), 1, chat_log);
			fprintf(chat_log, "%s mandou uma mensagem: %s\n", connection_descriptor.chat_ip, rcv_msg);
			fputc('\0', chat_log);
			fseek(chat_log, 0, SEEK_SET);
			fputc('1', chat_log);
		}
		else{
			fseek(chat_log, -1, SEEK_END);
			fprintf(chat_log, "%s mandou uma mensagem: %s\n", connection_descriptor.chat_ip, rcv_msg);
			fputc('\0', chat_log);
		}
		sem_post(&sem_file);
		bzero(rcv_msg, 1001);
	}
}

/*******************************************************************************
 *	NOME:		serverfunc
 *	FUNÇÃO:		Thread do servidor. Realizara todas as funcoes ligadas ao servidor.
 *
 *	RETORNO:	void
 *******************************************************************************/
void serverfunc(){
	
	char str[100];
	int listen_fd, comm_fd;
	
	listenerthreadparameters *new_connection;

	struct sockaddr_in servaddr, clientaddr;

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);

	if(listen_fd == -1){
		perror("Erro na criacao do socket");
	}

	int enable = 1;
	if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int) < 0))
		//printf("setsockopt(SO_REUSEADDR) failed");

	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(PORTA);

	if(bind(listen_fd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0){
		perror("Houve erro no bind");
	}

	listen(listen_fd, MAXHOSTS);

	int clientaddrlen = sizeof(struct sockaddr_in);
	while( comm_fd = accept(listen_fd, (struct sockaddr*) &clientaddr, (socklen_t*)&clientaddrlen) )
	{
		//conexao aceita, criando listener
		pthread_t listenerthread;
		new_connection = (listenerthreadparameters*)malloc(sizeof(listenerthreadparameters));
		new_connection->tempsock = comm_fd;
		strcpy( new_connection->chat_ip, inet_ntoa(clientaddr.sin_addr) );
		
		if(pthread_create(&listenerthread, NULL, serverlistener, (void*) new_connection) < 0 )
			perror("Erro, thread ouvinte nao criada");
		
	}//listener criado
	
	if(comm_fd < 0){
		printf("Erro ao aceitar conexao no servidor");
	}

    close(listen_fd);
}

void exclude_contacts(){
	int i, erro;
		char option;
		char stringAux[50];
		printf("Qual contato deseja excluir?\n1 - Endereco IPv4 do contato\n2 - Nome do contato\n");
		//Se host for o servidor na conexao
		erro = 1;
		__fpurge(stdin);
		do{
			__fpurge(stdin);
			option = getchar();
			if(option != '1' && option != '2')
				printf("Voce digitou uma opcao invalida. Tente:\n1 - Endereco IPv4 do contato\n2 - Nome do contato\n");
		}while(option != '1' && option != '2');

		if(option == '1'){
			printf("Digite o endereco IPv4\n");
			__fpurge(stdin);
			fgets(stringAux, 16, stdin);
			for(i = 0; i < numdecontatos; i++){
				if(strcmp(hostslist[i].hostip, stringAux) == 0 && hostslist[i].exist == 1){
					hostslist[i].exist = 0;
					contato = i;
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
				if(strcmp(hostslist[i].hostname, stringAux) == 0 && hostslist[i].exist == 1){
					hostslist[i].exist = 0;
					contato = i;
					erro = 0;
					break;
				}
			}
		}
		if (erro == 0){
			client_exclude = 1;
		}
		else{
			printf("Contato nao encontrado\n");
			sem_post(&sem_client);
		}
}

/*******************************************************************************
 *	NOME:		send_message
 *	FUNÇÃO:		Enviar mensagem. Ela verifica na lista de hosts se o usuario para
 *				o qual se deseja enviar a mensagem e seu cliente ou servidor.
 *				A partir disso, manda a informacao para thread cliente ou servidor.
 *
 *	RETORNO:	void
 *******************************************************************************/
void send_message(){
	int i, erro;
	char option;
	char stringAux[50];
	printf("Para quem gostaria de mandar a mensagem?\n1 - Endereco IPv4 do destinatario\n2 - Nome do destinatario\n");
	//Se host for o servidor na conexao
	erro = 1;

	do{
		__fpurge(stdin);
		option = getchar();
		if(option != '1' && option != '2')
			printf("Voce digitou uma opcao invalida. Tente:\n1 - Endereco IPv4 do contato\n2 - Nome do contato\n");
	}while(option != '1' && option != '2');

	if(option == '1'){
		printf("Digite o endereco IPv4\n");
		__fpurge(stdin);
		fgets(stringAux, 16, stdin);
		for(i = 0; i < numdecontatos; i++){
			if(strcmp(hostslist[i].hostip, stringAux) == 0 && hostslist[i].exist == 1){
				contato = i;
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
			if(strcmp(hostslist[i].hostname, stringAux) == 0 && hostslist[i].exist == 1){
				contato = i;
				erro = 0;
				break;
			}
		}
	}
	if (erro == 0){
		client_send = 1;
	}
	else{
		printf("Contato nao encontrado\n");
		sem_post(&sem_client);
	}
}

void refresh_messages(){
	int pos;		//posicao no arquivo de dados
	char check;		//checa se o arquivo esta vazio ou nao
	char option;	//lida com o menu
	char buffer[100];
	int end;

	do{
		printf("Deseja:\n1-Imprimir ultimas mensagens\n2-Imprimir todas as mensagens\n3-Excluir historico completo\n4-Sair\n");

		__fpurge(stdin);
		option = getchar();
		__fpurge(stdin);

		if(option != '4'){
			sem_wait(&sem_file);

			fseek(chat_log, 0, SEEK_END);
			end = ftell(chat_log);
			fseek(chat_log, 0, SEEK_SET);

			//O primeiro caracter do arquivo indica se ele esta vazio ou nao. 0 = vazio, 1 = nao vazio.
			check = fgetc(chat_log);
			if(check == '0'){
				printf("Voce nao recebeu nenhuma mensagem. Nao ha nada para imprimir ou excluir. D:\n");
			}
			else{

				if(option == '1'){
					fread(&pos, sizeof(int), 1, chat_log);
					fseek(chat_log, pos, SEEK_SET);
					if(ftell(chat_log) == end){
						printf("Voce nao tem nenhuma nova mensagem\n");
					}
					else{
						do{
							fread(&buffer, sizeof(buffer), 1, chat_log);
							printf("%s", buffer);
						}while(ftell(chat_log) != end);
					}
					fseek(chat_log, 0, SEEK_END);
					pos = ftell(chat_log);
					fseek(chat_log, 1, SEEK_SET);
					fwrite(&pos, sizeof(int), 1, chat_log);
				}
				else if(option == '2'){
					fread(&pos, sizeof(int), 1, chat_log);
					do{
						fread(buffer, sizeof(buffer), 1, chat_log);
						printf("%s", buffer);
					}while(ftell(chat_log) != end);
					fseek(chat_log, 0, SEEK_END);
					pos = ftell(chat_log);
					fseek(chat_log, 1, SEEK_SET);
					fwrite(&pos, sizeof(int), 1, chat_log);
				}
				else if(option == '3'){
					fseek(chat_log, 0, SEEK_SET);
					fputc('0', chat_log);
				}
			}
			sem_post(&sem_file);
		}
	}while(option != '4');
	sem_post(&sem_client);
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
		if(hostslist[i].exist == 1){
			printf("%s", hostslist[i].hostname);
			printf("\t\t%s\n", hostslist[i].hostip);
		}
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
		printf("Deseja:\n1-Inserir Contato\n2-Listar Contatos\n3-Excluir Contato\n4-Enviar Mensagem\n5-Mensagem em Grupo\n6-Sair\n7-Acessar Mensagens Recebidas\n");
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
				exclude_contacts();
				sem_wait(&sem_client);
				break;

			case 4:
				send_message();
				//Dorme enquanto espera a thread cliente executar.
				sem_wait(&sem_client);
				break;

			case 5:
				client_send = 2;
				sem_wait(&sem_client);
				break;

			case 6:
				printf("Adeus!\n");
				break;

			case 7:
				refresh_messages();
				sem_wait(&sem_client);
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
	sem_init(&sem_file, 0, 1);
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
	numdecontatos = 0;
	char filename[25];

	pthread_t serverthread, clientthread, menuthread;

	printf("Digite o numero da porta da aplicacao: ");
	scanf("%d", &PORTA);

	__fpurge(stdin);
	printf("Digite o seu endereco de IP: ");
	fgets(filename, 16, stdin);
	strtok(filename, "\n");
	__fpurge(stdin);

	strcat(filename, "chat.txt");


	chat_log = fopen(filename, "w+b");

	if(chat_log == NULL){
		printf("Erro na abertura do arquivo");
	}
	else{
		fputc('0', chat_log);
	}

	init_semaphores();
	erro = init_threads(&serverthread, &clientthread, &menuthread);
	if (erro == 1){	//Significa que as threads nao foram criadas. Logo o programa nao pode prosseguir.
		return 1;
	}
	else{
		pthread_join(menuthread, NULL);
	    pthread_join(clientthread, NULL);
		return 0;
	}
}
