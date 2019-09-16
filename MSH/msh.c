
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
#define MAX_NUM_ARGUMENTS 11    // Mav shell only supports 10 arguments, eleven to account for filename

#define MAX_PIDS          15
#define MAX_HISTORY       50

#define EXIT              42

// Used to track up to 15 pids
struct pids {
    pid_t listing[MAX_PIDS];
    int offset;
};

// Used to track up to 50 command
struct history {
    char listing[MAX_HISTORY][MAX_COMMAND_SIZE];
    int offset;
};

//  For storing BG pid
//  On every fork(), this variable gets updated with the latest PID.
pid_t CURRENT_BG_PROCESS = -1;

int input_handler(char *cmd_str, struct history *hist, struct pids *pids);

int pid_push(struct pids *pids, pid_t p);

void pid_show(struct pids *pids);

int history_push(struct history *hist, char *token);

void history_show(struct history *hist);

void nth_command(struct history *hist, struct pids *pids, char *token[MAX_COMMAND_SIZE]);

int exec_cmd(char *token[], struct pids *pids);

int interpreter(char **token, struct history *history, struct pids *pids);

// doesnt do anything
void signal_handler(int signal) {}

int bg();

void trim_whitespace(char *s);

int main() {

    char *cmd_str = (char *) malloc(MAX_COMMAND_SIZE);

    struct history history;
    history.offset = 0;

    struct pids pids;
    pids.offset = 0;

    // https://www.linuxprogrammingblog.com/code-examples/sigaction
    struct sigaction act;
    memset (&act, '\0', sizeof(act));
    act.sa_handler = &signal_handler;
    if (sigaction(SIGINT , &act, NULL) < 0){
        perror ("sigaction: ");
        return 1;
    }
    if (sigaction(SIGTSTP , &act, NULL) < 0) {
        perror ("sigaction: ");
        return 1;
    }

    while (1) {
        printf("msh> ");

        while (!fgets(cmd_str, MAX_COMMAND_SIZE, stdin));

        history_push(&history, cmd_str);

        int ret = input_handler(cmd_str, &history, &pids);

        if (ret == EXIT) {
            free(cmd_str);
            exit(0);
        }
    }
}

/**
 * Takes raw input for shell, splits shell input by ";",
 * then splits each partition into character array delimited by whitespace
 *
 * It delegates each character array to the interpreter function which decides
 * what action to commit
 * @param cmd_str raw input
 * @param hist history ptr
 * @param pids pids ptr
 * @return 0 on success
 *         != 0 if something went horribly wrong.
 */
int input_handler(char *cmd_str, struct history *hist, struct pids *pids) {
    int ret;
    // split the cmd _ str by ';'
    // execute each partition invidiually
    char *cpy = strndup(cmd_str, MAX_COMMAND_SIZE);
    char *addr = cpy;
    char *partition;
    while ((partition = strsep(&cpy, SPLIT)) != NULL) {
        // each iteration, is a unique partition of cmd_str delimited by ;
        // at this level we can break the partition into pieces delimited by " "
        trim_whitespace(partition);

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
        free(working_root);
        if (ret == EXIT)
            break;


    }

    free(addr);
    return ret;
}

/**
 * Pushes a new PID to the pid struct.
 * If the array is at max capacity, we must begin overwriting
 * elements from the beginning of the array, increasing
 * @param pids pids ptr
 * @param p pid to push
 * @return -1 if *pid is NULL
 *          0 on success.
 */
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

/**
 * Prints the pid struct
 * Because we begin to overwrite pids at the beginning of array when it gets too big,
 * we have to print the entries after the "pivot" index (pivot -> end), then print the elements
 * (0 -> pivot -1)
 * @param pids pids ptr
 */
void pid_show(struct pids *pids) {
    if (pids == NULL)
        return;
    if (pids->offset == 0)
        return;

    int numbering = 0, i;
    if (pids->offset % MAX_PIDS != pids->offset) {
        for (i = pids->offset % MAX_PIDS; i < MAX_PIDS; i++) {
            int pos = i % MAX_PIDS;
            printf("%d:\t%d\n", numbering, (int) pids->listing[pos]);
            numbering++;
        }
    }
    for (i = 0; i < pids->offset % MAX_PIDS; i++) {
        printf("%d:\t%d\n", numbering, (int) pids->listing[i]);
        numbering++;
    }
}

/**
 * Does essentially the same as pids_push()
 * @param hist hist ptr
 * @param token string to write
 * @return -1 if ptr is null
 *          0 on success
 */
int history_push(struct history *hist, char *token) {
    if (hist == NULL)
        return -1;

    int write_to = hist->offset % MAX_HISTORY;

    strncpy(hist->listing[write_to], token, MAX_COMMAND_SIZE);
    hist->offset++;
    return 0;
}

/**
 * Same concept of pids_show()
 * @param hist history ptr.
 */
void history_show(struct history *hist) {
    if (hist == NULL)
        return;
    if (hist->offset == 0)
        return;

    // numbering tracks the list order int for both loops.
    int numbering = 0, i;

    if (hist->offset % MAX_HISTORY != hist->offset) {
        for (i = hist->offset % MAX_HISTORY; i < MAX_HISTORY; i++) {
            int pos = i % MAX_HISTORY;
            printf("%d:\t%s", numbering, hist->listing[pos]);
            numbering++;
        }
    }
    for (i = 0; i < hist->offset % MAX_HISTORY; i++) {
        printf("%d:\t%s", numbering, hist->listing[i]);
        numbering++;
    }
}
/**
 * Per the requirements, the max N is 15, meaning the only commands that can
 * get reexecuted at from indexes [0...14]
 *
 * Therefore we need not to do any fancy % index operations, we can access the cmd normally
 * @param hist
 * @param pids
 * @param token
 */
void nth_command(struct history *hist, struct pids *pids, char *token[MAX_COMMAND_SIZE]) {
    int n = (int) strtol(&token[0][1], NULL, 10);
    int max = 15;
    if (n > max || n > hist->offset) {
        printf("Command not found in history.\n");
        return;
    }

    // access command in history at N, and send it to input handler function
    char *cpy = strndup(hist->listing[n], MAX_COMMAND_SIZE);
    input_handler(cpy, hist, pids);
}

/**
 * Accepts a string array, and decides what to do with it
 * @param token char **
 * @param history history ptr
 * @param pids pids ptr
 * @return EXIT if token[0]==("quit" || "exit")
 *         0 if everything works
 *         something not 0 is something breaks here
 */
int interpreter(char **token, struct history *history, struct pids *pids) {
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
    return exec_cmd(token, pids);
}

/**
 * Accepts string array, executes it as a commmand using exec()
 * @param token char **
 * @param pids pids ptr
 * @return see exec() doc
 */
int exec_cmd(char *token[], struct pids *pids) {
    int exec_status, status, i, num_paths = 4;
    char *search_paths[] = {
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
        for (i = 0; i < num_paths; i++) {
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
    waitpid(child_status, &status, 0);
    return exec_status;
}

/**
 * Resumes a BG process, if there is one.
 * @return see kill()
 */
int bg() {
    if (CURRENT_BG_PROCESS == -1) {
        printf("No process to resume\n");
        return 0;
    }
    // run kill() with SIGCONT to resume paused process
    int r = kill(CURRENT_BG_PROCESS, SIGCONT);
    if (r == -1)
        perror("KILL:");
    CURRENT_BG_PROCESS = -1;
    return r;
}

/**
 * Sometimes when uesrs enter many commands delimited ";" like this:
 * msh> echo foo; echo bar
 * foo
 *
 * bar will not be printed as there is trailing whitespace before "echo bar"
 * this function aims to remove that trim that whitespace
 *
 * @param s string to modify;
 */
void trim_whitespace(char *s) {
    char * cpy = strndup(s, MAX_COMMAND_SIZE);

    while (*cpy == ' ')
        *cpy++;

    strcpy(s, cpy);
}
