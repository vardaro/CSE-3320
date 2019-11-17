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

#include <sys/stat.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
// so we need to define what delimits our tokens.
// In this case  white space
// will separate the tokens on our command line

#define NUM_BLOCKS 512
#define BLOCK_SIZE 1024



#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments


// Directory struct
struct __attribute__((__packed__)) DirectoryEntry {
  char DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t Unused1[8];
  uint16_t DIR_FirstClusterHigh;
  uint8_t Unused2[4];
  uint16_t DIR_FirstClusterLow;
  uint32_t DIR_FileSize;
};

// fat32 spec struct
struct fat32 {
  char BS_OEMNAME[8];
  short BPB_BytsPerSec; // short
  unsigned BPB_SecPerClus;
  short BPB_RsvdSecCnt;
  unsigned BPB_NumFATS;
  short BPB_RootEntCnt;
  char BS_VolLab[11];
  int BPB_FATSz32;
  int BPB_RootClus;

  int RootDirSectors;
  int FirstDataSector;
  int FirstSectorofCluster;

  // storing our root offset
  int root_offset;
  int TotalFATSize;
  int bytesPerCluster;
};

unsigned char file_data[NUM_BLOCKS][BLOCK_SIZE];

int main()
{

    char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

    while( 1 )
    {
        // Print out the mfs prompt
        printf ("mfs> ");

        // Read the command from the commandline.  The
        // maximum command that will be read is MAX_COMMAND_SIZE
        // This while command will wait here until the user
        // inputs something since fgets returns NULL when there
        // is no input
        while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

        /* Parse input */
        char *token[MAX_NUM_ARGUMENTS];

        int   token_count = 0;

        // Pointer to point to the token
        // parsed by strsep
        char *arg_ptr;

        char *working_str  = strdup( cmd_str );

        // we are going to move the working_str pointer so
        // keep track of its original value so we can deallocate
        // the correct amount at the end
        char *working_root = working_str;

        // Tokenize the input stringswith whitespace used as the delimiter
        while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) &&
                (token_count<MAX_NUM_ARGUMENTS))
        {
            token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
            if( strlen( token[token_count] ) == 0 )
            {
                token[token_count] = NULL;
            }
            token_count++;
        }

        // Now print the tokenized input as a debug check
        // \TODO Remove this code and replace with your shell functionality

        int token_index  = 0;
        for( token_index = 0; token_index < token_count; token_index ++ )
        {
            printf("token[%d] = %s\n", token_index, token[token_index] );
        }

        free( working_root );

    }
    return 0;
}

int put(char * str) {
    int status;
    struct stat buf;

    status = stat(str, &buf);


    if (status == -1) {
        printf("Cannot resolve file %s", str);
        return 1;
    }

    FILE * ifp = fopen(str, "r");

    int copy_size = buf.st_size;
    int offset = 0;
    int block_index = 0;
    while(copy_size > 0) {
        fseek(ifp, offset, SEEK_SET);

        int bytes = fread(file_data, BLOCK_SIZE, 1, ifp);
        
        if (bytes == 0 && !feof(ifp)) {
            printf("Error occured reading from input file.");
            return -1;
        }

        clearerr(ifp);

        copy_size = copy_size - BLOCK_SIZE;

        offset = offset + BLOCK_SIZE;

        block_index++;

    }

    fclose(ifp);

    printf("File written");


}