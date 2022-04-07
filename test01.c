#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#define PAGE_SIZE 256 // in bytes
#define FRAME_SIZE (PAGE_SIZE) // in bytes
#define BACKING_STORE_SIZE (pow(2, 16)) // in bytes

static char *physical_memory[FRAME_SIZE];

void display_page(int page_num);

int main()
{
    FILE *fd = fopen("BACKING_STORE.bin", "rb");
    if (fd == NULL) {
        printf("file cannot be opened\n");
        return 1;
    }

    // char *buffer;
    // buffer = (char *) malloc(sizeof(char));

    // fseek(fd, 135, SEEK_SET);
    // fread(buffer, sizeof(char), 999, fd);

    // printf("sizeof(buffer) = %d\n", sizeof(buffer));
    // printf("sizeof(char) = %d\n", sizeof(char));
    // printf("%s\n", buffer);
    // printf("%c\n", buffer[0]);

    for (int i = 0; i < (BACKING_STORE_SIZE / FRAME_SIZE); i++)
    {
        char buffer[FRAME_SIZE];
        fseek(fd, i * FRAME_SIZE, SEEK_SET);
        int read = fread(buffer, sizeof(char), FRAME_SIZE, fd);
        physical_memory[i] = (char *) calloc(FRAME_SIZE, sizeof(char));
        if (read > 0) {
            strncpy(physical_memory[i], buffer, FRAME_SIZE);
            // for (int x = 0; x < FRAME_SIZE; x++)
            // {
            //     physical_memory[i][x] = buffer[x];
            // }
        }
        memset(buffer, 0, FRAME_SIZE);
    }
    display_page(0);


    // char *buffer;
    // buffer = (char *) calloc(256, sizeof(char));
    // int read = 0;
    // int i;
    // int addr = 0x000000;
    // read = fread(buffer, sizeof(char), 256, fd);
    // if (read > 0) {
    //     printf("read = %d\n", read);
    //     for(i = 0; i < read; i++)
    //     {
    //         if (buffer[i] == 0 || buffer[i] == NULL || buffer[i] == '\0')
    //             printf("%c ", buffer[i]);
    //         else
    //             printf("%c ", buffer[i]);
    //         // if (((i+1) % 0x10) == 0) {
    //         //     printf("\n");
    //         // }
    //     }
    //     printf("\n");
    //     // printf("%s\n", buffer);
    // } else {
    //     printf("Error occured while reading from .bin file\n");
    //     return 1;
    // }
    // fseek(fd, 0x80, SEEK_SET);

    // while((read = fread(buffer, 1, 256, fd)) > 0)
    // {
        // printf("read = %d\n", read);
        // printf("%s\n", buffer);
        // for(i = 0; i < read; i++)
        // {
        //     printf("%s",buffer[i], buffer[i]);
        // }
        // memset(buffer, 0, 32);
        // printf("\n");
        // char boom[32];
        // scanf("%X", boom);
    // }

    return 0;
}

void display_page(int page_num)
{
    for (int i = 0; i < PAGE_SIZE; i++)
    {
        printf("%02x ", physical_memory[page_num][i]);
    }
    printf("\n");
}