#include <iostream>
#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h> 

#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

#define PORT "3434"
#define PORT_NUM 3434
#define IP_SIZE 39 //maximum IPv6 address size
#define MAXDATASIZE 100
#define NAME_LEN 20

int newfd, fd_listen, fd_send, numbytes;
char buf[MAXDATASIZE];
char name[NAME_LEN];
struct addrinfo hints, *servinfo, *p;
int rv;
char s[INET6_ADDRSTRLEN], t[INET6_ADDRSTRLEN];



void say_hello(){
	cout << "Hello from client" << endl;
}

void exit_program(){
	exit(0);
}

void client_message(){
	int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT_NUM);
	serverAddress.sin_addr.s_addr = INADDR_ANY;	

	connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
	
	const char* message = "Hello, server!";
	send(clientSocket, message, strlen(message), 0);

	close(clientSocket);
}

void set_server_ip(){
	FILE* file;
	char ip[IP_SIZE];
	file = fopen("server_address", "w");
	if (file == NULL){
		perror("error: can't open server address file");
	} else {
		printf("enter new server ip address\n");	
		scanf("%s", ip);
		fprintf(file, "%s", ip);
		fclose(file);
		printf("done");
	}	
}

void *get_in_addr(struct sockaddr *sa){
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void server_connect(){	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	FILE* file;
	file = fopen("server_address", "r");
	fgets(t, INET6_ADDRSTRLEN, file);
	
	if ((rv = getaddrinfo(t, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	}
	
	for (p = servinfo; p != NULL; p=p->ai_next) {
		if ((newfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(newfd, p->ai_addr, p->ai_addrlen) == -1){
			close(newfd);
			perror("client: connect");
			continue;
		}
		usleep(50); //maybe delete?

		if ((fd_send = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			close(newfd);
			perror("client: socket - send");
			continue;
		}

		if (connect(fd_send, p->ai_addr, p->ai_addrlen) == -1) {
			close(newfd);
			close(fd_send);
			perror("client: connect - send");
		}
		usleep(50); //maybe delete?

		if ((fd_listen = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
			close(newfd);
			close(fd_send);
			perror("client: socket - listen");
			continue;
		}

		if (connect(fd_listen, p->ai_addr, p->ai_addrlen) == -1) {
			close(newfd);
			close(fd_send);
			close(fd_listen);
			perror("client: listen");
			continue;
		}
		printf("listen socket established\n");
		break;
		
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		exit(1);
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo);
	
	if ((numbytes = recv(newfd, buf, MAXDATASIZE-1, 0)) == -1) {
		perror("recv initial");
		exit(1);
	}	
	usleep(50);

	if (send(newfd, name, NAME_LEN, 0) == -1)
		perror("send name");

	buf[numbytes] = '\0';
	printf("client: received '%s'\n", buf);
	
	while(1){
		scanf("%s", &buf);
		if (send(fd_send, buf, MAXDATASIZE, 0) == -1)
			perror("send error");
	}
	close(newfd);
	close(fd_send);
	close(fd_listen);

}
