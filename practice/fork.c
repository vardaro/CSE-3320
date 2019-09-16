#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

int main(void) {
    pid_t pid_1;
    pid_t pid_2;
    int status;

    pid_1 = fork();
    if (!pid_1) {
        wait(&status);

        printf("Tatooine\n");
        if (!(pid_2 = fork())) {
            wait(&status);
            printf("Kashyyk\n");
        } else {
            wait(&status);
            printf("Hoth\n");
        }
        exit(0);
    }

    wait(&status);
    printf("Dagobah\n");
    exit(0);
}