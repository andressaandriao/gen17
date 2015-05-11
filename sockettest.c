#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

main ()
{
	int sock, cli;
	struct sockaddr_in server, client;
	unsigned int len;//makes len strictly positive
	char mesg[] = "Hello World!";
	int sent;//number of bytes sent
	
	if( (sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{//socket call returns -1 as error
		perror("socket: ");
		exit(-1);
	}
	
	server.sin_family = AF_INET;
	server.sin_port = htons(10000);//host to network short -> hton
	server.sin_addr.s_addr = INADDR_ANY;
	bzero(&server.sin_zero, 8);
	
	len = sizeof(struct sockaddr_in);
	
	if((bind(sock, (struct sockaddr *)&server, len)) == -1)
	{
		perror("bind");
		exit(-1);
	}
	
	if((listen(sock, 5)) == -1)
	{
		perror("listen");
		exit(-1);
	}
	
	while(1)
	{
		if( (cli = accept(sock, (struct sockaddr*)&client, &len) ) == -1)//accept is a blocking call
		{
			perror("accept");
			exit(-1);
		}
		
		sent = send(cli, mesg, strlen(mesg), 0);
		printf("Sent %d bytes to client: %d\n", sent, inet_ntoa(client.sin_addr) );//network to host ascii
		
		close(cli);
	}

}
//abrir no linux em dois terminais; o primeiro roda este programa, o segundo digitara telnet 10000
