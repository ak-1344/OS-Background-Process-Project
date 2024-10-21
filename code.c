#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

void ask_user_confirmation() {
    char command[512];
    snprintf(command, sizeof(command), 
             "zenity --question --text='Do you want to continue tracking app usage?' --title='User Confirmation'");

    int status = system(command);
    if (status != 0) {
        printf("Tracking terminated by user.\n");
        exit(EXIT_SUCCESS);
    }
}

void background_task() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process
        execlp("./exec_process", "process", (char *)NULL);
        perror("execlp failed");
        exit(EXIT_FAILURE);
    }
}

int main() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS); // Parent exits
    }

    if (setsid() < 0) {
        perror("Failed to create a new session");
        exit(EXIT_FAILURE);
    }
	int first_time=1;
    while (1) {
    	if(first_time!=1){
		ask_user_confirmation();
    	}
        background_task();
        sleep(60); // Sleep for 60 seconds before the next confirmation
        first_time++;
    }

    return 0;
}
