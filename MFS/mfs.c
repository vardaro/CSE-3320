// MIT License (MIT)
//
// Copyright (c) 2016, 2017 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software wit     restriction, including without limitation the rights
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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdint.h>

#include <sys/stat.h>

#define WHITESPACE " \t\n" // We want to split our command line up into tokens
// so we need to define what delimits our tokens.
// In this case  white space
// will separate the tokens on our command line

#define NUM_BLOCKS 4226
#define BLOCK_SIZE 8192
#define NUM_FILES 128
#define MAX_FILE_SIZE 1024000

FILE *fd; // file descriptor for image
char image[225];

uint8_t blocks[NUM_BLOCKS][BLOCK_SIZE];

#define MAX_COMMAND_SIZE 255 // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5 // Mav shell only supports five arguments

enum directory_state {
    FREE,
    NOT_FREE
};

struct directory_entry {
    uint8_t valid;
    char filename[255];
    uint32_t inode;
};

struct inode {
    uint8_t hidden;
    uint8_t readonly;
    uint8_t valid;
    int size;
    uint32_t blocks[1250];
};

struct directory_entry *dir;
struct inode *inodes;
uint8_t *free_block_list;
uint8_t *free_inode_list;

/**
 * createfs command
 * @param filename name of the image to be create
 */
void init_fs(char *filename) {
    FILE * fd = fopen(filename, "w");

    memset(&blocks[0], 0, NUM_BLOCKS * BLOCK_SIZE);

    fwrite(&blocks[0], NUM_BLOCKS, BLOCK_SIZE, fd);

    fclose(fd);

}

/**
 * Initialized directory entries
 * Each one is FREE at first, and have no associated inode index.
 * Null filename
 */
void init_dir() {
    int i;
    for (i = 0; i < NUM_FILES; i++) {
        dir[i].valid = FREE;
        dir[i].inode = -1;

        memset(dir[i].filename, 0, 255);
    }
}

/**
 * Init block list to be all FREE
 */
void init_block_list() {
    int i;
    for (i = 0; i < NUM_BLOCKS; i++) {
        free_block_list[i] = FREE;
    }
}

/**
 * Init inodes to all be FREE
 */
void init_inode_list() {
    int i;
    for (i = 0; i < NUM_FILES; i++) {
        free_inode_list[i] = FREE;
    }
}

/**
 * Init all inode objects to their default values
 * memset the block indexess to -1
 */
void init_inodes() {
    int i;
    for (i = 0; i < NUM_FILES; i++) {
        inodes[i].valid = FREE;
        inodes[i].size = 0;
        inodes[i].hidden = 0;
        inodes[i].readonly = 0;
        int j;
        for (j = 0; j < 1250; j++) {
            inodes[i].blocks[j] = -1;
        }
    }
}

/**
 * Computes how much free space left
 * @return int denoting disk space
 */
int disk_space() {
    int i;
    int free_space = 0;
    for (i = 0; i < NUM_BLOCKS; i++) {
        if (free_block_list[i] == FREE) {
            free_space = free_space + BLOCK_SIZE;
        }
    }

    return free_space;
}

/**
 * Returns index of directory entry
 * @param filename name to find
 * @return int denoting index
 */
int find_directory_by_name(char *filename) {
    int i;
    int ret = -1;
    for (i = 0; i < NUM_FILES; i++) {
        if (strcmp(filename, dir[i].filename) == 0) {
            return i;
        }
    }
    return ret;
}

/**
 * Returns an index for filename
 * if there is not directory entry for that name,
 * it makes a new one
 * @param filename
 * @return int denoting index
 */
int find_directory_index(char *filename) {
    // check is filename is already there
    int i;
    int ret = find_directory_by_name(filename);

    if (ret != -1)
        return ret;

    // find free disk space
    for (i = 0; i < NUM_FILES; i++) {
        if (dir[i].valid == FREE) {
            dir[i].valid = NOT_FREE;
            strncpy(dir[i].filename, filename, 255);
            return i;
        }
    }

    return ret;
}

/**
 * Loops over inode list, returns the first FREE one
 * @return int denoting inode index
 */
int find_free_inode() {
    int i;
    int ret = -1;

    for (i = 0; i < NUM_FILES; i++) {
        if (inodes[i].valid == FREE) {
            inodes[i].valid = NOT_FREE;
            return i;
        }
    }

    return ret;
}

/**
 * Same as find_free_inode() for blocks
 * @return int denoting block idx.
 */
int find_free_block() {
    int i;
    int ret = -1;

    for (i = 0; i < NUM_BLOCKS; i++) {
        if (free_block_list[i] == FREE) {
            free_block_list[i] = NOT_FREE;
            return i;
        }
    }

    return ret;
}

/**
 * Puts file into memory
 * @param filename file to grab
 * @return 0 if no errors
 *         1 if something went horribly wrong
 */
int put(char *filename) {
    struct stat buf;
    int temp;

    temp = stat(filename, &buf);

    if (temp == -1) {
        printf("put error: File does not exist!!!!\n");
        return -1;
    }

    int size = buf.st_size;

    if (size > MAX_FILE_SIZE) {
        printf("put error: File too large\n");
        return -1;
    }

    if (size > disk_space()) {
        printf("put error: Not enough disk space\n");
        return -1;
    }

    // put the file in the img

    FILE *ifp = fopen(filename, "r");

    int dir_index = find_directory_index(filename);
    int inode_index = find_free_inode();


    int offset = 0;
    int block_ptr_offset = 0;

    int block_index;
    inodes[inode_index].size = size;

    while (size > 0) {
        block_index = find_free_block();
        fseek(ifp, offset, SEEK_SET);


        int bytes = fread(blocks[block_index], BLOCK_SIZE, 1, ifp);

        if (bytes == 0 && !feof(ifp)) {
            printf("An error occured reading from the input file.\n");
            return -1;
        }

        clearerr(ifp);

        size -= BLOCK_SIZE;

        offset += BLOCK_SIZE;

        // Increment the index into the block array
        inodes[inode_index].blocks[block_ptr_offset] = block_index;
        block_ptr_offset++;

    }


    fclose(ifp);
    strcpy(dir[dir_index].filename, filename);
    dir[dir_index].inode = inode_index;
    dir[dir_index].valid = NOT_FREE;
    return 0;
}

/**
 * gets file out of memory into target file
 *
 * @param filename file to grab
 * @param newfilename file to go to
 * @return 0 if no errors
 *         1 if something went horribly wrong
 */
int get(char *filename, char *newfilename) {
    FILE *ofp;
    ofp = fopen(newfilename, "w");


    if (ofp == NULL) {
        printf("Could not open output file: %s\n", filename);
        perror("Opening output file returned");
        return -1;
    }

    int offset = 0;

    int dir_index = find_directory_by_name(filename);

    if (dir_index == -1) {
        printf("get error: File not found\n");
        return -1;
    }

    int inode_index = dir[dir_index].inode;
    int size = inodes[inode_index].size;

    int block_counter = 0;
    int block_index = inodes[inode_index].blocks[block_counter];
    while (size > 0) {

        int bytes;

        if (size < BLOCK_SIZE) {
            bytes = size;
        } else {
            bytes = BLOCK_SIZE;
        }

        fwrite(blocks[block_index], bytes, 1, ofp);

        size -= BLOCK_SIZE;
        offset += BLOCK_SIZE;

        block_counter++;
        block_index = inodes[inode_index].blocks[block_counter];
//        block_index++;
    }

    fclose(ofp);

    return 0;
}


/**
 * Wipes a files off the face of the earth
 * @param filename file to delete
 * @return 0 if A-OK
 *         1 if not A-OK
 */
int del(char * filename) {
    int dir_index = find_directory_by_name(filename);
    int inode_index = dir[dir_index].inode;


    // check if the file is read only, if so, we cant delete it
    if (inodes[inode_index].readonly == 1) {
        printf("del: File is read only, can't be deleted sorry\n");
        return 1;
    }

    // at the inodes, set every block pointer to by the inode to free
    int i;
    for (i = 0; i < sizeof(inodes[inode_index].blocks); i++) {
        int cur = inodes[inode_index].blocks[i];

        // if the current iteration is -1, we can stop early
        if (inodes[inode_index].blocks[i] == -1)
            break;

        // update the free block list, and set the block index to -1
        free_block_list[cur] = FREE;
        inodes[inode_index].blocks[i] = -1;

    }

    inodes[inode_index].valid = FREE;
    inodes[inode_index].size = 0;
    inodes[inode_index].hidden = 0;
    inodes[inode_index].readonly = 0;

    dir[dir_index].valid = FREE;
    dir[dir_index].inode = -1;

    memset(dir[dir_index].filename, 0, 255);

    return 0;

}

/**
 * Shows the files in memory
 * It's supposed to show a timestamp, I honestly couldn't get around to it
 */
void list() {
    int i;
    int flag = 0;

    for (i = 0; i < NUM_FILES; i++) {
        // if valid and not hidden print it
        if (dir[i].valid == NOT_FREE && inodes[dir[i].inode].hidden == 0) {
            int inode_idex = dir[i].inode;
            printf("%s %d\n", dir[i].filename, inodes[inode_idex].size);
            flag++;
        }
    }

    if (flag == 0) {
        printf("list: No files found.\n");
    }
}

/**
 * Opens a disk image
 * @param filename .image to open
 */
void open_fs(char *filename) {
    FILE * fd = fopen(filename, "r+");
    memset(blocks, 0, NUM_BLOCKS * BLOCK_SIZE);

    fread(blocks, NUM_BLOCKS, BLOCK_SIZE, fd);

     fclose(fd);

     strncpy(image, filename, 255);
}

/**
 * Closes the currently open disk image
 */
void close_fs() {
    FILE * fd = fopen(image, "w");

    fwrite(blocks, NUM_BLOCKS, BLOCK_SIZE, fd);

    fclose(fd);

    image[0] = '\0';
}

/**
 * Applies an attribute to a file
 * @param attrib attrib to apply
 * @param filename filename to violate
 */
void attrib(char * attrib, char * filename) {
    int dir_index = find_directory_by_name(filename);
    if (dir_index == -1) {
        printf("attrib: File not found\n");
        return;
    }

    // Find the correct inode;
    int inode_index = dir[dir_index].inode;

    // Mark the file accordingly
    if (strncmp(attrib, "+h", MAX_COMMAND_SIZE) == 0) {
        inodes[inode_index].hidden = 1;
        return;
    }

    if (strncmp(attrib, "-h", MAX_COMMAND_SIZE) == 0) {
        inodes[inode_index].hidden = 0;
        return;
    }

    if (strncmp(attrib, "+r", MAX_COMMAND_SIZE) == 0) {
        inodes[inode_index].readonly = 1;
        return;
    }

    if (strncmp(attrib, "-r", MAX_COMMAND_SIZE) == 0) {
        inodes[inode_index].readonly = 0;
        return;
    }

    printf("attrib: Bad input\n");

}

int main() {


    dir = (struct directory_entry *) &blocks[0];
    inodes = (struct inode *) &blocks[3];

    free_inode_list = (uint8_t *) &blocks[1];
    free_block_list = (uint8_t *) &blocks[2];

    init_dir();
    init_block_list();
    init_inode_list();
    init_inodes();

    char *cmd_str = (char *) malloc(MAX_COMMAND_SIZE);
    while (1) {
        // Print out the mfs prompt
        printf("mfs> ");

        // Read the command from the commandline.  The
        // maximum command that will be read is MAX_COMMAND_SIZE
        // This while command will wait here until the user
        // inputs something since fgets returns NULL when there
        // is no input
        while (!fgets(cmd_str, MAX_COMMAND_SIZE, stdin));

        /* Parse input */
        char *token[MAX_NUM_ARGUMENTS];

        int token_count = 0;

        // Pointer to point to the token
        // parsed by strsep
        char *arg_ptr;

        char *working_str = strdup(cmd_str);

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

        if (token[0] == NULL) {
            printf("Cannot parse input\n");
        } else if (strncmp(token[0], "put", MAX_COMMAND_SIZE) == 0) {
            put(token[1]);
        } else if (strncmp(token[0], "get", MAX_COMMAND_SIZE) == 0) {
            if (token_count == 4) {
                get(token[1], token[2]);
            } else {
                get(token[1], token[1]);
            }
        } else if (strncmp(token[0], "quit", MAX_COMMAND_SIZE) == 0) {
            break;
        } else if (strncmp(token[0], "del", MAX_COMMAND_SIZE) == 0) {

            del(token[1]);
        } else if (strncmp(token[0], "list", MAX_COMMAND_SIZE) == 0) {
            list();
        } else if (strncmp(token[0], "df", MAX_COMMAND_SIZE) == 0) {
            printf("%d bytes free.\n", disk_space());
        } else if (strncmp(token[0], "open", MAX_COMMAND_SIZE) == 0) {
            open_fs(token[1]);

        } else if (strncmp(token[0], "close", MAX_COMMAND_SIZE) == 0) {
            close_fs();

        } else if (strncmp(token[0], "attrib", MAX_COMMAND_SIZE) == 0) {
            if (token_count != 4) {
                printf("Need a attribute and a file name\n");
                continue;
            }
            attrib(token[1], token[2]);
        } else if (strncmp(token[0], "createfs", MAX_COMMAND_SIZE) == 0) {
            init_fs(token[1]);
        }

        free(working_root);
    }
    return 0;
}
