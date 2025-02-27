#include "client_commands.cpp"

typedef void (*commandFunc)();
typedef struct {
	string command;
	commandFunc function;
} Command;

Command commands[] = {
	{"hello", say_hello},
	{"message", client_message},
	{"setip", set_server_ip},
	{"connect", server_connect},
	{"exit", exit_program}
};

void execute_command(string command){
	for (size_t i = 0; i < sizeof(commands)/sizeof(commands[0]); i++){
		if (command == commands[i].command){
			commands[i].function();
			return;
		}
	}
}

void command_listen(){
	string command;
	while(1){
		cin >> command;
		execute_command(command);
	}
}

void setup_user(char *str){
	printf("connected as: %s\n", str);
	strcpy(name, str);
	printf("%s\n", name);
}

int main(int argc, char *argv[]){
	if (argc != 2) {
		fprintf(stderr, "use: ./client [name]\n");
		exit(0);
	}
	setup_user(argv[1]);
	command_listen();
	return 0;
}
