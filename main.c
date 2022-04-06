#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef unsigned char BYTE;

#define BACKING_STORE_FILENAME      "BACKING_STORE.bin"
#define LOGICAL_ADDRESSES_FILENAME  "addresses.txt"
#define BUFFER_LEN                  256
#define NUM_PAGE_TABLE_ENTRY        256                 // 2^8
#define NUM_PHYS_MEM_ENTRY          (NUM_PAGE_TABLE_ENTRY)
#define PAGE_SIZE                   256                 // 2^8 in bytes
#define EMPTY_PAGE_ENTRY            0
#define VALID_PAGE_ENTRY            1
#define INVALID_PAGE_ENTRY          0
#define FRAME_SIZE                  (PAGE_SIZE)
#define NUM_TLB_TABLE_ENTRY         16


int logical_addresses_size = 1;
unsigned int *logical_addresses;
unsigned int **page_table;

void read_addresses(const char *filename, int *const size,unsigned int *logical_addresses);
int get_page_number(int bin_val);
int get_offset(int bin_val);
unsigned int **init_page_table(unsigned int **tbl);
unsigned char *physical_memory[FRAME_SIZE];



int main(int argc, char **argv)
{
    FILE *fd = fopen(BACKING_STORE_FILENAME, "rb");
    for (int i = 0; i < NUM_PHYS_MEM_ENTRY; i++)
    {
        physical_memory[i] = (char *) malloc(sizeof(char));
    }
    init_page_table(page_table);
    // return 0;
    logical_addresses = (unsigned int *) malloc(logical_addresses_size * sizeof(unsigned int));
    read_addresses(LOGICAL_ADDRESSES_FILENAME, &logical_addresses_size, logical_addresses);

    printf("%d %d %d\n",logical_addresses[2], get_page_number(logical_addresses[2]), get_offset(logical_addresses[2]));

    
    // for (int y = 0; y < NUM_PAGE_TABLE_ENTRY; y++)
    // {
    //     fseek(fd, get_page_number(logical_addresses[y]) * FRAME_SIZE, SEEK_SET);
    //     fread(physical_memory[get_page_number(logical_addresses[y])], sizeof(char), FRAME_SIZE, fd);
    //     printf("%d %s\n",y, physical_memory[get_page_number(logical_addresses[y])]);
    // }
    free(logical_addresses);
    return 0;
}

void read_addresses(const char *filename, int *const size, unsigned int *arr)
{
    FILE *fd = fopen(filename, "r");
    char buffer[BUFFER_LEN];
    int curr_index = *size - 1;
    while (fgets(buffer, BUFFER_LEN, fd) != NULL)
    {
        arr = (unsigned int *) realloc(arr, (curr_index + 1) * sizeof(unsigned int));
        *(arr + curr_index) = (unsigned int) atoi(buffer);
        printf("%d = %d\n", curr_index, arr[curr_index]);
        memset(buffer, 0, BUFFER_LEN);
        curr_index++;
    }
    *size = curr_index + 1;
    logical_addresses = arr;
}

int get_page_number(int bin_val)
{
    // logical address binary format
    // 32-bit integer numbers represents logical addresses
    // 00000000 00000000 (00000000) 00000000
    //                    page num

    // shift 8 bits to the right and mask the first 8 bits
    return ((bin_val >> 8) & 0xff);
}
int get_offset(int bin_val)
{
    // logical address binary format
    // 32-bit integer numbers represents logical addresses
    // 00000000 00000000 00000000 (00000000)
    //                             offset
    
    // mask first 8 bits
    return (bin_val & 0xff);
}

unsigned int **init_page_table(unsigned int **tbl)
{
    if (!tbl) {
        tbl = (unsigned int **) calloc(NUM_PAGE_TABLE_ENTRY, sizeof(unsigned int*));
        for (int i = 0; i < NUM_PAGE_TABLE_ENTRY; i++)
        {
            tbl[i] = (unsigned int *) calloc(2, sizeof(unsigned int));
            tbl[i][0] = EMPTY_PAGE_ENTRY;
            tbl[i][1] = INVALID_PAGE_ENTRY;
        }
    }

    // for (int x = 0; x < NUM_PAGE_TABLE_ENTRY; x++)
    //     {
    //         printf("%d %d %d\n", x, tbl[x][0], tbl[x][1]);
    //     }
    return tbl;
}