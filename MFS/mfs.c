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

FILE * fd; // file descriptor for image

uint8_t blocks[NUM_BLOCKS][BLOCK_SIZE];

#define MAX_COMMAND_SIZE 255 // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5 // Mav shell only supports five arguments

enum directory_state
{
    FREE,
    NOT_FREE
};

enum fn_state
{
    OK,
    ERROR
};

struct directory
{
    uint8_t valid;
    char filename[255];
    uint32_t inode;
};

struct inode
{
    uint8_t attribute;
    uint8_t valid;
    uint8_t size;
    uint32_t blocks[1250];
};

struct directory *dir;
struct inode *inodes;
uint8_t *free_block_list;
uint8_t *free_inode_list;

// fat32 spec struct
// https://www.win.tue.nl/~aeb/linux/fs/fat/fat-1.html

unsigned char file_data[NUM_BLOCKS][BLOCK_SIZE];

void init_fs(char * filename) {
    fd = fopen(filename, "w");

    memset(&blocks[i], 0, NUM_BLOCKS * BLOCK_SIZE);

    fwrite(&blocks[0], NUM_BLOCKS, BLOCK_SIZE, fd);

    fclose(fd);
}


void init_dir()
{
    int i;
    for (i = 0; i < NUM_FILES; i++)
    {
        dir[i].valid = FREE;
        dir[i].inode = -1;

        memset(dir[i].filename, 0, 255);
    }
}

void init_block_list()
{
    int i;
    for (i = 0; i < NUM_BLOCKS; i++)
    {
        free_block_list[i] = FREE;
    }
}

void init_inode_list()
{
    int i;
    for (i = 0; i < NUM_FILES; i++)
    {
        free_inode_list[i] = FREE;
    }
}

void init_inodes()
{
    int i;
    for (i = 0; i < NUM_FILES; i++)
    {
        inodes[i].valid = NOT_FREE;
        inodes[i].size = 0;
        inodes[i].attribute = 0;

        int j;
        for (j = 0; j < 1250; j++)
        {
            inodes[i].blocks[j] = -1;
        }
    }
}

int disk_space()
{
    int i;
    int free_space = 0;
    for (i = 0; i < NUM_BLOCKS; i++)
    {
        if (free_block_list[i] == FREE)
        {
            free_space = free_space + BLOCK_SIZE;
        }
    }

    return free_space;
}

int find_directory_index(char *filename)
{
    // check is filename is already there
    int i;
    int ret = ERROR;
    for (i = 0; i < NUM_FILES; i++)
    {
        if (strcmp(filelength, dir[i].filename) == 0)
        {
            return i;
        }
    }

    // find free disk space
    for (i = 0; i < NUM_FILES; i++)
    {
        if (dir[i].valid == FREE)
        {
            dir[i].valid = NOT_FREE;
            return i;
        }
    }

    return ret;
}

int find_free_inode()
{
    int i;
    int ret = -1;

    for (i = 0; i < NUM_FILES; i++)
    {
        if (inodes[i].valid == FREE)
        {
            inodes[i].valid = NOT_FREE;
            return i;
        }
    }

    return ret;
}

int find_free_block()
{
    int i;
    int ret = ERROR;

    for (i = 0; i < NUM_BLOCKS; i++)
    {
        if (free_block_list[i] == FREE)
        {
            free_block_list[i] = NOT_FREE;
            return i;
        }
    }

    return ret;
}


int put(char *filename)
{
    struct stat buf;
    int temp;

    temp = stat(filename, &buf);

    if (temp == -1)
    {
        printf("File does not exist!!!!\n");
        return ERROR;
    }

    int size = buf.st_size;

    if (size > MAX_FILE_SIZE)
    {
        printf("File of size %d is too large\n", size);
        return ERROR;
    }

    if (size > disk_space())
    {
        printf("Not enough space to store file\n");
        return ERROR;
    }

    // put the file in the img

    int dir_index = find_directory_index(filename);
    int inode_index = find_free_inode();
}

void list() {
    int i;
    for (i = 0; i < NUM_FILES; i++) {
        // if valid and not hidden print it
        if (dir[i].valid == NOT_FREE) {
            int inode_idex = dir[i].inode;
            printf("%s %s\n", dir[i].filename, inodes[inode_idex].size);
        }
    }
}

int main()
{

    init_fs("disk.image");

    // hey, compiler, i REALLY want to cast this thing to a directory :)
    dir = (struct directory *)&blocks[0];
    inodes = (struct inode *)&blocks[3];

    free_inode_list = (uint8_t *)&blocks[1];
    free_block_list = (uint8_t *)&blocks[2];

    init_dir();
    init_block_list();
    init_inode_list();
    init_inodes();

    char *cmd_str = (char *)malloc(MAX_COMMAND_SIZE);

    while (1)
    {
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
               (token_count < MAX_NUM_ARGUMENTS))
        {
            token[token_count] = strndup(arg_ptr, MAX_COMMAND_SIZE);
            if (strlen(token[token_count]) == 0)
            {
                token[token_count] = NULL;
            }
            token_count++;
        }

        // Now print the tokenized input as a debug check
        // \TODO Remove this code and replace with your shell functionality

        int token_index = 0;
        for (token_index = 0; token_index < token_count; token_index++)
        {
            printf("token[%d] = %s\n", token_index, token[token_index]);
        }

        free(working_root);
    }
    return 0;
}
