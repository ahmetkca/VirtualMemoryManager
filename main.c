#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define MASK_VALID_INVALID_BIT      ((unsigned int)0x80000000)
#define BACKING_STORE_FILENAME      "BACKING_STORE.bin"
#define LOGICAL_ADDRESSES_FILENAME  "addresses.txt"
#define BUFFER_LEN                  256
#define NUM_PAGE_TABLE_ENTRY        256                 // 2^8
#define NUM_PHYS_MEM_ENTRY          (NUM_PAGE_TABLE_ENTRY)
#define PAGE_SIZE                   256                 // 2^8 in bytes
#define EMPTY_PAGE_ENTRY            0
#define VALID_PAGE_ENTRY            0x80000000
#define INVALID_PAGE_ENTRY          0x00000000
#define FRAME_SIZE                  (PAGE_SIZE)
#define NUM_TLB_TABLE_ENTRY         16
#define BACKING_STORE_SIZE          ((unsigned int)(0x10000))


int logical_addresses_size = 1;
unsigned int *logical_addresses;
unsigned int page_table[NUM_PAGE_TABLE_ENTRY];
unsigned char *physical_memory;

void read_addresses(const char *filename, int *const size,unsigned int *logical_addresses);
int get_page_number(int bin_val);
int get_offset(int bin_val);
unsigned int *init_page_table(unsigned int *tbl);
void swap_in(unsigned int page_num);
// void update_page_table(unsigned int page_num, )



int main(int argc, char **argv)
{
    FILE *fd = fopen(BACKING_STORE_FILENAME, "rb");

    // initialize page table
    init_page_table(page_table);
    
    // read logical address from addresses.txt
    logical_addresses = (unsigned int *) malloc(logical_addresses_size * sizeof(unsigned int));
    read_addresses(LOGICAL_ADDRESSES_FILENAME, &logical_addresses_size, logical_addresses);

    printf("%d, %d\n", get_page_number(logical_addresses[0]), get_offset(logical_addresses[0]));
    return 0;

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
        // printf("%d = %d\n", curr_index, arr[curr_index]);
        memset(buffer, 0, BUFFER_LEN);
        curr_index++;
    }
    *size = curr_index + 1;
    logical_addresses = arr;
}

int get_page_number(int bin_val)
{
    /*
     *  logical address binary format
     *  32-bit integer numbers represents logical addresses
     *  00000000 00000000 (00000000) 00000000
     *                     page num
     */

    // shift 8 bits to the right and mask the first 8 bits
    return ((bin_val >> 8) & 0xff);
}
int get_offset(int bin_val)
{
    /*
     *  logical address binary format
     *  32-bit integer numbers represents logical addresses
     *  00000000 00000000 00000000 (00000000)
     *                              offset
     */                
    
    // mask first 8 bits
    return (bin_val & 0xff);
}

unsigned int *init_page_table(unsigned int *tbl)
{
    /*
     *  page table contains unsigned integer (32-bit)
     *  most left bit represent the validity of the page entry
     *  valid page entry represented by     10000000 00000000 00000000 00000000 (binary)
     *                                      0x80000000                          (hexadecimal)
     *  invalid page entry represented by   00000000 00000000 00000000 00000000 (binary)
     *                                      0x00000000                          (hexadecimal)
     */ 
    if (!tbl) {
        tbl = (unsigned int *) calloc(NUM_PAGE_TABLE_ENTRY, sizeof(unsigned int*));
        for (int i = 0; i < NUM_PAGE_TABLE_ENTRY; i++)
        {
            tbl[i] = 0 | VALID_PAGE_ENTRY;
        }
    } else {
        for (int i = 0; i < NUM_PAGE_TABLE_ENTRY; i++)
        {
            tbl[i] = 0 | VALID_PAGE_ENTRY;
        }
    }

    // for (int x = 0; x < NUM_PAGE_TABLE_ENTRY; x++)
    // {
    //     printf("%08x \n", tbl[x]);
    // }
    return tbl;
}

void swap_in(unsigned int page_num)
{
    FILE *fd = fopen(BACKING_STORE_FILENAME, "rb");
    char buffer[FRAME_SIZE];
    fseek(fd, page_num * FRAME_SIZE, SEEK_SET);
    int read_;
    fread(buffer, sizeof(unsigned char), FRAME_SIZE, fd);
    if (!physical_memory[page_num])
        physical_memory[page_num] = (unsigned char *) calloc(FRAME_SIZE, sizeof(unsigned char));
    strncpy(physical_memory[page_num], buffer, FRAME_SIZE);
    fclose(fd);
}