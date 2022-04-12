#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "tlb.h"


#define MASK_VALID_BIT      ((unsigned int)0x80000000)
#define MASK_FRAME_NUMBER   ((unsigned int)0x7fffffff)
#define MASK_OFFSET         (0x000000ff)
#define MASK_PAGE_NUM       (0x0000ff00)
#define OUTPUT_FILENAME             ""
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

static unsigned int tlb_hit_rate = 0;
static unsigned int page_fault = 0;
static unsigned int current_frame_number = 0;
static int logical_addresses_size = 1;
static unsigned int *logical_addresses;
static unsigned int *page_table;
static unsigned char *physical_memory;


void read_logical_addresses(const char *filename, int *const size,unsigned int * const logical_addresses);
unsigned int get_page_number(int bin_val);
unsigned int get_offset(int bin_val);
unsigned int *init_page_table(unsigned int * const tbl);
int swap_in(unsigned int page_num, unsigned char * const mem, unsigned int * const page_table, unsigned int *curr_frm_num);
void write_to_physical_memory(unsigned char *buff, unsigned int offset, unsigned char * const mem);
void update_current_frame_num(unsigned int *curr_frm_num);
void update_page_table(unsigned int page_num, int frame_num, unsigned int * const page_table);
unsigned int get_frame_address_from_page_table(unsigned int page_num, const unsigned int * const page_table);
unsigned int consult_page_table(unsigned int page_num, bool *is_valid, const unsigned int * const page_table);
void check_page_table_entry_validity(unsigned int page_num, bool *is_valid, const unsigned int * const page_table);
unsigned char physical_memory_seek(unsigned int phys_addr, const unsigned char * const mem);
unsigned int generate_phys_addr_translation(unsigned int frame_addr, unsigned int offset);


int main(int argc, char **argv)
{
    g_tlb = init_tlb();
    page_table = (unsigned int *) malloc(NUM_PAGE_TABLE_ENTRY*sizeof(unsigned int));
    physical_memory = (unsigned char *) malloc(BACKING_STORE_SIZE * sizeof(unsigned char));

    // initialize page table
    init_page_table(page_table);
    
    // read logical address from addresses.txt
    logical_addresses = (unsigned int *) malloc(logical_addresses_size * sizeof(unsigned int));
    read_logical_addresses(LOGICAL_ADDRESSES_FILENAME, &logical_addresses_size, logical_addresses);

    // printf("%d, %d\n", get_page_number(logical_addresses[0]), get_offset(logical_addresses[0]));

    for (int i = 0; i < logical_addresses_size; i++)
    {
        unsigned int page_n = get_page_number(logical_addresses[i]);
        unsigned int offset = get_offset(logical_addresses[i]);
        printf("Virtual address = %*u, page number = %*u, offset = %*u, ", 4, logical_addresses[i], 4, page_n, 4, offset);
        bool is_valid;
        unsigned int frame_addr;


        tlb_entry_t *tlb_result = look_up(g_tlb, page_n);
        if (tlb_result == NULL)     // TLB Miss 
        {
            frame_addr = consult_page_table(page_n, &is_valid, page_table);
            if (is_valid == false)
            {
                page_fault++;
                swap_in(page_n, physical_memory, page_table, &current_frame_number);
                frame_addr = consult_page_table(page_n, &is_valid, page_table);
                unsigned int phys_addr_trans = generate_phys_addr_translation(frame_addr, offset);
                printf("physical address translation %*u, ", 8, phys_addr_trans);
                char ret_val = physical_memory_seek(phys_addr_trans, physical_memory);
                printf("Value = %*d\n", 4, (int) ret_val);
            } else {
                unsigned int phys_addr_trans = generate_phys_addr_translation(frame_addr, offset);
                printf("physical address translation %*u, ", 8,  phys_addr_trans);
                char ret_val = physical_memory_seek(phys_addr_trans, physical_memory);
                printf("Value = %*d\n", 4, (int) ret_val);
            }
            enqueue(g_tlb, page_n, frame_addr);
        } 
        else  // TLB Hit
        {
            tlb_hit_rate++;
            unsigned int frame_addr = get_frame_addr(tlb_result);
            unsigned int phys_addr_trans = generate_phys_addr_translation(frame_addr, offset);
            printf("physical address translation %*u, ", 8,  phys_addr_trans);
            char ret_val = physical_memory_seek(phys_addr_trans, physical_memory);
            printf("Value = %*d\n", 4, (int) ret_val);
        }
    }

    printf("Number of translated addresses = %u\n", logical_addresses_size);
    printf("Number of Page Faults = %u\n", page_fault);
    printf("Page Fault rate = %0.3f \n", (float)(((int)page_fault) / (float) logical_addresses_size));
    printf("Number of TLB Hits = %u\n", tlb_hit_rate);
    printf("TLB Hit rate = %0.3f\n", (float)(((int)tlb_hit_rate) / (float) logical_addresses_size));
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

/**
 * @brief                   Extracts page number from 32-bit unsigned integer (logical address)
 *                          00000000 00000000 (00000000) 00000000
 * 
 * @param bin_val           Virtual/logical address in the form of 32-bit unsigned integer
 * @return unsigned int     page number max 8-bit (255)
 */
unsigned int get_page_number(int bin_val)
{
    /*
     *  00000000 00000000 (00000000) 00000000
     *                     page num
     */

    // shift 8 bits to the right and mask the first 8 bits
    return ((bin_val >> 8) & 0xff);
}

/**
 * @brief               Extract offset number (8-bit (max 255)) from virtual/logical address by masking last 8 bit.
 * 
 * @param bin_val       Virtual/logical address (32-bit unsigned integer)
 * @return              offset number (32-bit unsigned integer but max val is 255 (8-bits))
 */
unsigned int get_offset(int bin_val)
{
    /*
     *  00000000 00000000 00000000 (00000000)
     *                              offset
     */                
    
    // mask first 8 bits
    return (bin_val & 0xff);
}

/**
 * @brief               Initialize the page table, every entry's most significant bit is set to INVALID
 *                      and rest of bits are set to 0 to inditicate empty page table entry
 * 
 * @param tbl               page table struct
 * @return unsigned int*    it returns pointer to the initialized page table struct
 */
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

/**
 * @brief               It handles the page fault that occurs when a cpu requests a frame address from page table but
 *                      the given page number is not present in the page table. The page corresponds to the page number
 *                      should bring in from Secondary Storage/HDD/Backing Store into the physical memory (ex. RAM).
 *                      Page table should be updated with the newly brought in frame address.
 * 
 * @param page_num      Page number associated with the frame address
 * @param mem           Implementation of physical memory
 * @param page_table    Implementation of page table
 * @param curr_frm_num  Current available frame number 
 * @return int          It returns 0  to indicate swap in opreation was successful, and -1 otherwise.
 */
int swap_in(unsigned int page_num, unsigned char * const mem, unsigned int * const page_table, unsigned int *curr_frm_num)
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
    update_current_frame_num(curr_frm_num);
    fclose(fd);
    return 0;
}

/**
 * @brief           Writes a char buffer that has the length FRAME_SIZE to the given physical memory,
 *                  starting from given offset.
 * 
 * @param buff      character array length of FRAME_SIZE
 * @param offset    number that indicates the starting position of the write operation in the physical memory
 * @param mem       Implementation/Representation of the physical memory (ex. 4096 bytes long unsigned char array)
 */
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
void update_current_frame_num(unsigned int *curr_frm_num)
{
    *curr_frm_num = ((*curr_frm_num) + 1) % NUM_PHYS_MEM_ENTRY;
}

/**
 * @brief               Update the page table entry associated with the provided page number
 *                      with the given frame address (starting position of the frame in the physical memory)
 * 
 * @param page_num      Virtual page number that will be mapped to corresponding frame address
 * @param frame_addr    Starting position of the frame in the physical memory
 * @param page_table    Implementation of the page table (ex. array of unsigned integer with each entry's MSB indicates valid-invalid)
 */
void update_page_table(unsigned int page_num, int frame_addr, unsigned int * const page_table)
{
    page_table[page_num] = frame_addr | MASK_VALID_BIT;
}

/**
 * @brief               Get the frame address from page table with the provided page number (index)
 * 
 * @param page_num      Page number extracted from virtual/logical address in the form of unsigned integer, MSB is discarded.
 * @param page_table    Implementation of the page table (ex. array of unsigned integer with each entry's MSB indicates valid-invalid)
 * @return unsigned int It returns the frame address in the form of unsigned integer MSB is discarded since it it used for invalid-valid bit
 */
unsigned int get_frame_address_from_page_table(unsigned int page_num, const unsigned int * const page_table)
{
    return page_table[page_num] & MASK_FRAME_NUMBER;
}

/**
 * @brief               It is a look up function to check if the given page number is valid or invalid in the page table
 *                      , then return the frame address if valid, or page fault occurs otherwise.
 * 
 * @param page_num      A unsigned integer extracted from Virtual/logical address, MSB is discarded.
 * @param is_valid      A integer to store if the look up was valid or invalid.
 * @param page_table    Implementation of the page table (ex. array of unsigned integer with each entry's MSB indicates valid-invalid)
 * @return unsigned int Frame address corresponding to the provided page number.
 */
unsigned int consult_page_table(unsigned int page_num, bool *is_valid, const unsigned int * const page_table)
{
    check_page_table_entry_validity(page_num, is_valid, page_table);
    if ((*is_valid) == true)
        return get_frame_address_from_page_table(page_num, page_table);
    return -1;
}

/**
 * @brief               Check if the provided page number is valid (which means the page is in the physical memory and no page fault)
 *                      or invalid (which means the requested logical address is not mapped to physical address yet and swap in operation might be needed)
 *   
 * @param page_num      A unsigned integer extracted from virtual/logical address max 8-bit in length.
 * @param is_valid      A integer var to store the validity of the function, 1 indicates requested page is in hte physical memory, 0 indicates requested page is not in the physical memory
 * @param page_table    Implementation of the page table (ex. array of unsigned integer with each entry's MSB indicates valid-invalid)
 */
void check_page_table_entry_validity(unsigned int page_num, bool *is_valid, const unsigned int * const page_table)
{
    if ((page_table[page_num] & MASK_VALID_BIT) == MASK_VALID_BIT) {
        *is_valid = true;
        return;
    }
    *is_valid = false;
}

/**
 * @brief                   Get the 1-byte value at the given physical address (frame address + offset) in the physical memory.
 * 
 * @param phys_addr         Physical address equals to frame address (starting position of the frame) plus offset.
 * @param mem               Physical memory (ex. unsigned char array length 4096 bytes (1 char is 1 bytes)).
 * @return unsigned char    1-byte value at the frame with offset applied.
 *                          
 *                              ┌─────────────┐
 *                              │             │
 *                              │             │
 *                              │             │
 *                              │             │   Frame 1
 *                              │             │
 *                              │             │
 *                              │             │
 *    Frame Address ────────►xxx├─────────────┤
 *                           x  │             │
 *                    Offset x  │             │
 *                           x  │             │
 * (frame addr + offset)     x  │             │   Frame 2
 *  Physical Address ───────►xxx│ ── ── ── ── │
 *                              │             │
 *                              │             │
 *                              └─────────────┘ 
 */
unsigned char physical_memory_seek(unsigned int phys_addr, const unsigned char * const mem)
{
    return *(mem + phys_addr);
    
}


/**
 * @brief               Adds the provided offset (with in FRAME_SIZE range) to the given Frame address
 * 
 * @param frame_addr    Mapped unsigned integer that indicates the starting position of the frame (associated with the page in the virtual space) in the physical memory.
 * @param offset        A unsigned integer represents a offset within given frame range (provided frame address + FRAME_SIZE)
 * @return unsigned int that represents a 1-byte value within the frame which resides in the physical memory.
 */
unsigned int generate_phys_addr_translation(unsigned int frame_addr, unsigned int offset)
{
    if (offset > FRAME_SIZE)
        return -1;
    return frame_addr + offset;
}