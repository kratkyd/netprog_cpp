#include "server_commands.cpp"

typedef void (*commandFunc)();

typedef struct {
	string command;
	commandFunc function;
} Command;

Command commands[] = {
	{"hello", say_hello},
	{"server", server_run},
	{"ip", show_ip}, 
	{"message", server_message},
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

int main(){
	initialize();
	command_listen();
	return 0;
}
