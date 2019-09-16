#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

int main(void) {
    // execl("/bin/mkdir", "/bin/mkdir", "GoT", NULL);
    pid_t child_pid = fork();
    if (child_pid == 0) {
        printf("Stark\n");
    } else {
        int status;
        int child_pid = fork();
        if (child_pid == 0) {
            int child_pid = fork();
            if (child_pid == 0) {
                printf("Arryn\n");
            } else {
                wait(&status);
                printf("Baratheon\n");
            }
        }
        else {
            int child_pid = fork();
            if (child_pid == 0) {
                printf("Lannister\n");
            } else  {
                wait(&status);
                printf("Greyjob\n");
            }
        }
        wait(&status);
        printf("Targaryen\n");
        exit(0);
    }
}