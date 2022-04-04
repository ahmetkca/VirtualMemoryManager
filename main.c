#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LOGICAL_ADDRESSES_FILENAME "addresses.txt"
#define BUFFER_LEN 256

void read_addresses(const char *filename, int *const size, int *logical_addresses);
void mask_rightmost_16bits(int *bin_num);
int get_page_number(int bin_val);
int get_offset(int bin_val);

int logical_addresses_size = 1;
int *logical_addresses;


int main(int argc, char **argv)
{
    logical_addresses = (int *) malloc(logical_addresses_size * sizeof(int));
    read_addresses(LOGICAL_ADDRESSES_FILENAME, &logical_addresses_size, logical_addresses);

    printf("%d\n", get_page_number(logical_addresses[0]));
    free(logical_addresses);
    return 0;
}

void read_addresses(const char *filename, int *const size, int *arr)
{
    FILE *fd = fopen(filename, "r");
    char buffer[BUFFER_LEN];
    int curr_index = *size - 1;
    while (fgets(buffer, BUFFER_LEN, fd) != NULL)
    {
        arr = (int *) realloc(arr, (curr_index + 1) * sizeof(int));
        *(arr + curr_index) = atoi(buffer);
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
    bin_val &= 0x0000ff00;
    return (bin_val << 16) >> 24;
}
int get_offset(int bin_val)
{
    // logical address binary format
    // 32-bit integer numbers represents logical addresses
    // 00000000 00000000 00000000 (00000000)
    //                             offset
    bin_val &= 0x000000ff;
    return (bin_val << 24) >> 24;
}