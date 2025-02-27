#include <iostream>
#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <poll.h>
#include <vector>

//imports from c (just coppied)
#include <sys/types.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>

#define PORT "3434"
#define PORT_NUM 3434
#define BACKLOG 10
#define MAXDATASIZE 500
#define NAME_LEN 20


int sockfd, new_fd, new_fds[BACKLOG]; //listen on sock_fd, new connections on new_fd	
int fd_listen[BACKLOG];
int fd_send[BACKLOG];
int pipe_fd[BACKLOG][2];
char users[BACKLOG][NAME_LEN];
int fd_num = 0;
struct addrinfo hints, *servinfo, *p;
struct sockaddr_storage their_addr;
std::vector<struct pollfd> fds(BACKLOG);

socklen_t sin_size;
struct sigaction sa;
int yes=1;
char s[INET6_ADDRSTRLEN];
int rv;

using namespace std;

//initialization
void initialize(){
	fill(fd_listen, fd_listen+BACKLOG, -2);
	fill(fd_send, fd_send+BACKLOG, -2);
	for (int i = 0; i < BACKLOG; ++i){
		fill(users[i], users[i]+NAME_LEN, 0);
	}
}


void say_hello(){
	cout << "Hello from function!" << endl;
}

void exit_program(){
	exit(0);
}

void *get_in_addr(struct sockaddr *sa){
	if(sa->sa_family == AF_INET){
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


void show_ip(){
	struct ifaddrs *ifaddr, *ifa;
	int family;
	char address[200] = "";

	if (getifaddrs(&ifaddr)==-1){
		perror("getifaddrs error");
		exit(1);
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa -> ifa_next){
		if (ifa -> ifa_addr == NULL){
			continue;
		}

		inet_ntop(ifa->ifa_addr->sa_family, get_in_addr((struct sockaddr *)ifa->ifa_addr), address, sizeof address);
		if (strcmp(ifa->ifa_name, "wlo1") || !strcmp(address, "")){
			continue;
		}
		printf("%s: %s\n", (ifa->ifa_addr->sa_family == AF_INET)?("IPv4"):("IPv6"), address);
	}

	freeifaddrs(ifaddr);
}

void server_message(){
	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT_NUM);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

	listen(serverSocket, 5);
	int clientSocket = accept(serverSocket, nullptr, nullptr);

	char buffer[1024] = {0};
	recv(clientSocket, buffer, sizeof(buffer), 0);
	cout << "Message from client: " << buffer << endl;

	close(serverSocket);
}

void sigchld_handler(int s){
	(void)s;

	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

int server_setup(){

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; //use my IP
	
	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for (p=servinfo; p!=NULL; p->ai_next){
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo);

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");
	return 0;
}

void process_message(int user_id, char message[MAXDATASIZE]){
	printf("%s: %s\n", users[user_id], message);
	for (int i = 0; i < BACKLOG; i++){
		if (fd_send[i] != -2){
			if (send(fd_send[i], message, MAXDATASIZE, 0)==-1)
				perror("send error");
		}
	}
}

int server_listen(){
	char buf[MAXDATASIZE];
	while(1) {
		sin_size = sizeof their_addr;
		
		if (fd_num < BACKLOG){
			new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
			if (new_fd == -1) {
				perror("accept");
				continue;
			}
		
			inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
			printf("server: got connection from %s\n", s);

			if (send(new_fd, "Hello there!", 13, 0) == -1)
				perror("send");
			if (recv(new_fd, buf, MAXDATASIZE-1, 0) == -1)
				perror("recv");
			printf("buf1: %s\n", buf);	

			for (int i = 0; i < BACKLOG; ++i){
				if (fd_listen[i] == -2 && fd_send[i] == -2){
					fd_listen[i] = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
					fd_send[i] = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

					strcpy(users[i], buf);
					fd_num++;
					break;
				}
			}
		} else {
			perror("BACKLOG full");
		}	
		
		for (int i = 0; i < BACKLOG; ++i){
			if (fd_listen[i] != -2){
				if( pipe(pipe_fd[i])<0){
					perror("pipe error");
					exit(1);
				}
				if (!fork()){ //add error possibility (use switch)
					close(pipe_fd[i][0]); //maybe delete? Might need to listen as well
					printf("buf: %s\n", buf);
					printf("users[i]: %s\n", users[i]);
					while(1){
						int msg_len = recv(fd_listen[i], buf, MAXDATASIZE-1, 0);
						if (msg_len == -1)
							perror("recv - child");
						if (msg_len == 0)
							exit(0);

						//functioning pipe, it was just stupid to use it
						//write(pipe_fd[i][1], buf, msg_len);
						process_message(i, buf);
						//exit(0);	
						//don't forget to kill the child
					}
				}// else {
				//	for (int i = 0; i < BACKLOG; i++){
				//		fds[i].fd = pipe_fd[i][0]; 	
				//		fds[i].events = POLLIN;
				//	}  

				//	while(true){
				//		int ready = poll(fds.data(), fds.size(), -1);
				//		if (ready == -1){
				//			perror("poll failed");
				//			break;
				//		}

				//		for (int i = 0; i < BACKLOG; i++){
				//			if (fds[i].revents & POLLIN){
				//				ssize_t bytesRead = read(fds[i].fd, buf, MAXDATASIZE);
                //				if (bytesRead > 0) {
                //    			buf[bytesRead] = '\0';
				//				process_message(i, buf);
                //				}
				//			}
				//		}
				//	}
				//}
			}
		}
		close(new_fd);
	}

	return 0;
}

void server_run(){
	server_setup();
	server_listen();
}
