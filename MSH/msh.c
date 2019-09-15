
// The MIT License (MIT)
//
// Copyright (c) 2016, 2017 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

/*
 * Name: Anthony Vardaro
 * ID:   1001522383
 */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define SPLIT      ";"
#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
// so we need to define what delimits our tokens.
// In this case  white space
// will separate the tokens on our command line

#define MAX_COMMAND_SIZE  255    // The maximum command-line size
#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

#define MAX_PIDS          15
#define MAX_HISTORY       5

#define EXIT              42

// Used to track up to 15 pids
struct pids {
    pid_t listing[MAX_PIDS];
    int offset;
};

// Used to track up to 50 command
struct history {
    char listing[MAX_HISTORY][255];
    int offset;
};

//  For storing BG pid
pid_t CURRENT_BG_PROCESS = -1;

int input_handler(char * cmd_str, struct history * hist, struct pids * pids);


int pid_push(struct pids *pids, pid_t p);

void pid_show(struct pids * pids);

int history_push(struct history *hist, char * token);

void history_show(struct history * hist);

void nth_command(struct history * hist, struct pids * pids, char * token[MAX_COMMAND_SIZE]);

int exec_cmd(char *token[], struct pids * pids);

int interpreter(char **token, struct history * history, struct pids *pids);

void signal_handler(int signal);

int bg();

int main() {

    char *cmd_str = (char *) malloc(MAX_COMMAND_SIZE);

    struct history history;
    history.offset = 0;

    struct pids pids;
    pids.offset = 0;

//  configure signal handler to catch ctrl-c and ctrl-z
    signal(SIGINT, signal_handler);
    signal(SIGTSTP, signal_handler);

    while (1) {
        // Print out the msh prompt
        printf("msh> ");

        while (!fgets(cmd_str, MAX_COMMAND_SIZE, stdin));

        history_push(&history, cmd_str);

        int ret = input_handler(cmd_str, &history, &pids);
        if (ret == EXIT)
            break;


    }
    return 0;
}

int input_handler(char * cmd_str, struct history * hist, struct pids * pids) {
    int ret;
    // split the cmd _ str by ';'
    // execute each partition invidiually
    char * cpy = strndup(cmd_str, MAX_COMMAND_SIZE);
    char * addr = cpy;
    char * partition;
    while ((partition = strsep(&cpy, SPLIT)) != NULL) {
        // each iteration, is a unique partition of cmd_str delimited by ;
        // at this level we can break the partition into pieces delimited by " "
        char *token[MAX_NUM_ARGUMENTS];
        memset(token, 0, sizeof(token));
        int token_count = 0;

        // Pointer to point to the token
        // parsed by strsep
        char *arg_ptr;

        char *working_str = strdup(partition);
        // we are going to move the working_str pointer so
        // keep track of its original value so we can deallocate
        // the correct amount at the end
        char *working_root = working_str;

        // Tokenize the input stringswith whitespace used as the delimiter
        while (((arg_ptr = strsep(&working_str, WHITESPACE)) != NULL) &&
               (token_count < MAX_NUM_ARGUMENTS)) {
            token[token_count] = strndup(arg_ptr, MAX_COMMAND_SIZE);
            if (strlen(token[token_count]) == 0) {
                token[token_count] = NULL;
            }
            token_count++;
        }

        ret = interpreter(token, hist, pids);
        if (ret == EXIT)
            break;

        free(working_root);

    }

    free(addr);
    return ret;
}
int pid_push(struct pids *pids, pid_t p) {
    if (pids == NULL)
        return -1;

    // divide the offset by the max number of elements,
    // get the remainder -- thats our index
    int write_to = pids->offset % MAX_PIDS;
    pids->listing[write_to] = p;
    pids->offset++;
    return 0;
}

void pid_show(struct pids * pids) {
    if (pids == NULL)
        return;
    if (pids->offset == 0)
        return;

    int numbering = 0;
    if (pids->offset % MAX_PIDS != pids->offset) {
        for (int i = pids->offset % MAX_PIDS; i < MAX_PIDS; i++) {
            int pos = i % MAX_PIDS;
            printf("%d:\t%d\n", numbering,(int)pids->listing[pos]);
            numbering++;
        }
    }
    for (int i = 0; i < pids->offset % MAX_PIDS; i++) {
        printf("%d:\t%d\n", numbering,(int)pids->listing[i]);
        numbering++;
    }
}


int history_push(struct history *hist, char * token) {
    if (hist == NULL)
        return -1;

    int write_to = hist->offset % MAX_HISTORY;

    strncpy(hist->listing[write_to], token, MAX_COMMAND_SIZE);
    hist->offset++;
    return 0;
}

void history_show(struct history * hist) {
    if (hist == NULL)
        return;
    if (hist->offset == 0)
        return;

    // numbering tracks the list order int for both loops.
    int numbering = 0;

    if (hist->offset % MAX_HISTORY != hist->offset) {
        for (int i = hist->offset % MAX_HISTORY; i < MAX_HISTORY; i++) {
            // if the user has not entered more than MAX_HISTORY commands
            // this condition will exit the loop on the first iteration,
            // and only the second loop will run.
            // if user has 10 commands (hist.offset = 10) with a max of 50,
            // 10 % 50 == 10 == hist.offset, break the loop

            int pos = i % MAX_HISTORY;
            printf("%d:\t%s", numbering, hist->listing[pos]);
            numbering++;
        }
    }
    for (int i = 0; i < hist->offset%MAX_HISTORY; i++) {
        printf("%d:\t%s", numbering, hist->listing[i]);
        numbering++;
    }
}

void nth_command(struct history * hist, struct pids * pids, char * token[MAX_COMMAND_SIZE]) {
    int n = (int)strtol(&token[0][1], NULL, 10);
    int max = 15;
    if (n > max || n > hist->offset) {
        printf("Command not found in history.\n");
        return;
    }


//    printf("%s", hist->listing[n]);

    // access command in history at N, and send it to input handler function
    char * cpy = strndup(hist->listing[n], MAX_COMMAND_SIZE);
    input_handler(cpy, hist, pids);
}

int interpreter(char **token, struct history * history, struct pids *pids) {
    if (token[0] == NULL)
        return 0;
    if (strncmp(token[0], "quit", MAX_COMMAND_SIZE) == 0 ||
        strncmp(token[0], "exit", MAX_COMMAND_SIZE) == 0) {
        return EXIT;
    } else if (strncmp(token[0], "listpids", MAX_COMMAND_SIZE) == 0) {
        // Show the last 15 process created
        pid_show(pids);
        return 0;
    } else if (strncmp(token[0], "cd", MAX_COMMAND_SIZE) == 0) {
        return chdir(token[1]);
    } else if (strncmp(token[0], "bg", MAX_COMMAND_SIZE) == 0) {
        return bg();
    } else if (strncmp(token[0], "history", MAX_COMMAND_SIZE) == 0) {
        history_show(history);
        return 0;
    } else if (token[0][0] == '!') {
        nth_command(history, pids, token);
        return 0;
    }


    // If the code reaches here then the user input gets passed along to the execvp handler
    return exec_cmd(token,pids);
}

int exec_cmd(char *token[], struct pids * pids) {
    int exec_status;
    int num_paths = 4;
    char * search_paths[] = {
            "/",
            "/usr/local/bin/",
            "/usr/bin/",
            "/bin/"
    };

    pid_t child_status = fork();
    // store the pid in the global var, in case the user backgrounds it
    CURRENT_BG_PROCESS = child_status;
    pid_push(pids, child_status);

    char cur[100];
    if (child_status == 0) {
        // child process
        for (int i = 0, len = num_paths; i < len; i++) {
            memset(cur, 0, sizeof(cur));
            strcpy(cur, search_paths[i]);
            strncat(cur, token[0], MAX_COMMAND_SIZE);

            fflush(stdout);
            exec_status = execv(cur, token);
            if (exec_status != -1)
                break;
        }

        printf("%s: Command not found.\n", token[0]);
    }
    wait(NULL);
    return exec_status;
}

int bg() {
    if (CURRENT_BG_PROCESS == -1)
        return 0;
    // run kill() with SIGCONT to resume paused process
    int r = kill(CURRENT_BG_PROCESS, SIGCONT);
    if (r == -1)
        perror("KILL:");
    CURRENT_BG_PROCESS = -1;
    return r;
}

void signal_handler(int signal) {
    //doesnt do much
    if (signal != SIGINT && signal != SIGTSTP)
        printf("Signal error\n");
    return;
}


#pragma clang diagnostic pop
