#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>


#define MASK_VALID_BIT      ((unsigned int)0x80000000)
#define MASK_FRAME_NUMBER   ((unsigned int)0x7fffffff)
#define MASK_OFFSET         (0x000000ff)
#define MASK_PAGE_NUM       (0x0000ff00)
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

static unsigned int current_frame_number;
static int logical_addresses_size = 1;
static unsigned int *logical_addresses;
static unsigned int *page_table;
static unsigned char *physical_memory;


void read_logical_addresses(const char *filename, int *const size,unsigned int * const logical_addresses);
unsigned int get_page_number(int bin_val);
unsigned int get_offset(int bin_val);
unsigned int *init_page_table(unsigned int * const tbl);
int swap_in(unsigned int page_num, unsigned char * const mem, unsigned int * const page_table);
void write_to_physical_memory(unsigned char *buff, unsigned int offset, unsigned char * const mem);
void update_current_frame_num();
void update_page_table(unsigned int page_num, int frame_num, unsigned int * const page_table);
unsigned int get_frame_address_from_page_table(unsigned int page_num, const unsigned int * const page_table);
unsigned int consult_page_table(unsigned int page_num, bool *is_valid, const unsigned int * const page_table);
void check_page_table_entry_validity(unsigned int page_num, bool *is_valid, const unsigned int * const page_table);
unsigned char physical_memory_seek(unsigned int phys_addr, const unsigned char * const mem);
unsigned int generate_phys_addr_translation(unsigned int frame_addr, unsigned int offset);



int main(int argc, char **argv)
{
    page_table = (unsigned int *) malloc(NUM_PAGE_TABLE_ENTRY*sizeof(unsigned int));
    physical_memory = (unsigned char *) malloc(BACKING_STORE_SIZE * sizeof(unsigned char));

    // initialize page table
    init_page_table(page_table);
    
    // read logical address from addresses.txt
    logical_addresses = (unsigned int *) malloc(logical_addresses_size * sizeof(unsigned int));
    read_logical_addresses(LOGICAL_ADDRESSES_FILENAME, &logical_addresses_size, logical_addresses);

    // printf("%d, %d\n", get_page_number(logical_addresses[0]), get_offset(logical_addresses[0]));

    for (int i = 0; i < 5; i++)
    {
        unsigned int page_n = get_page_number(logical_addresses[i]);
        unsigned int offset = get_offset(logical_addresses[i]);
        printf("Logical address = %u, page number = %u, offset = %u, ", logical_addresses[i], page_n, offset);
        bool is_valid;
        unsigned int frame_addr;
        frame_addr = consult_page_table(page_n, &is_valid, page_table);
        if (is_valid == false)
        {
            printf("Page fault occured, reading page num %d from backing store\n", page_n);
            swap_in(page_n, physical_memory, page_table);
            frame_addr = consult_page_table(page_n, &is_valid, page_table);
            unsigned int phys_addr_trans = generate_phys_addr_translation(frame_addr, offset);
            printf("physical address translation %u, ", phys_addr_trans);
            unsigned char ret_val = physical_memory_seek(phys_addr_trans, physical_memory);
            printf("Value = %u\n", (unsigned int) ret_val);
        } else {
            unsigned int phys_addr_trans = generate_phys_addr_translation(frame_addr, offset);
            printf("physical address translation %u, ", phys_addr_trans);
            unsigned char ret_val = physical_memory_seek(phys_addr_trans, physical_memory);
            printf("Value = %u\n", (unsigned int) ret_val);
        }
    }

    free(logical_addresses);
    return 0;
}

void read_logical_addresses(const char *filename, int *const size, unsigned int *arr)
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
    *size = curr_index;
    logical_addresses = arr;
}

unsigned int get_page_number(int bin_val)
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
unsigned int get_offset(int bin_val)
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
            tbl[i] = 0 | INVALID_PAGE_ENTRY;
        }
    } else {
        for (int i = 0; i < NUM_PAGE_TABLE_ENTRY; i++)
        {
            tbl[i] = 0 | INVALID_PAGE_ENTRY;
        }
    }

    // for (int x = 0; x < NUM_PAGE_TABLE_ENTRY; x++)
    // {
    //     printf("%08x \n", tbl[x]);
    // }
    return tbl;
}

int swap_in(unsigned int page_num, unsigned char * const mem, unsigned int * const page_table)
{
    unsigned int phys_mem_offset = current_frame_number * FRAME_SIZE;
    FILE *fd = fopen(BACKING_STORE_FILENAME, "rb");
    unsigned char buffer[FRAME_SIZE];
    fseek(fd, page_num * FRAME_SIZE, SEEK_SET);
    int read;
    read = fread(buffer, sizeof(unsigned char), FRAME_SIZE, fd);
    if (read <= 0) 
    {
        fclose(fd);
        return -1;
    }
    if (!mem)
        return -1;
    write_to_physical_memory(buffer, phys_mem_offset, mem);
    update_page_table(page_num, phys_mem_offset, page_table);
    update_current_frame_num();
    fclose(fd);
    return 0;
}

void write_to_physical_memory(unsigned char *buff, unsigned int offset, unsigned char *mem)
{
    for (int i = 0; i < FRAME_SIZE; i++)
    {
        *(mem + offset + i) = *(buff + i);
    }
}

/**
 * @brief Increment the current free frame number by 1 and don't exceed the maximum num of frame entry
 * 
 */
void update_current_frame_num()
{
    static unsigned int current_frame_number;
    current_frame_number = (current_frame_number + 1) % NUM_PHYS_MEM_ENTRY;
}

void update_page_table(unsigned int page_num, int frame_addr, unsigned int * const page_table)
{
    page_table[page_num] = frame_addr | MASK_VALID_BIT;
}

unsigned int get_frame_address_from_page_table(unsigned int page_num, const unsigned int * const page_table)
{
    return page_table[page_num] & MASK_FRAME_NUMBER;
}

unsigned int consult_page_table(unsigned int page_num, bool *is_valid, const unsigned int * const page_table)
{
    check_page_table_entry_validity(page_num, is_valid, page_table);
    if ((*is_valid) == true)
        return get_frame_address_from_page_table(page_num, page_table);
    return -1;
}

void check_page_table_entry_validity(unsigned int page_num, bool *is_valid, const unsigned int * const page_table)
{
    if ((page_table[page_num] & MASK_VALID_BIT) == MASK_VALID_BIT) {
        *is_valid = true;
        return;
    }
    *is_valid = false;
}

unsigned char physical_memory_seek(unsigned int phys_addr, const unsigned char * const mem)
{
    return *(mem + phys_addr);
    
}

unsigned int generate_phys_addr_translation(unsigned int frame_addr, unsigned int offset)
{
    if (offset > FRAME_SIZE)
        return -1;
    return frame_addr + offset;
}