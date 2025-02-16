#include <iostream>
#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h> 

using namespace std;

int port = 3333;

int main(){
	int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	serverAddress.sin_addr.s_addr = INADDR_ANY;	

	connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
	
	const char* message = "Hello, server!";
	send(clientSocket, message, strlen(message), 0);

	close(clientSocket);

	return 0;
}
