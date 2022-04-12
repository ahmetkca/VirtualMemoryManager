#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "gtest/gtest.h"
#include "tlb.h"


TEST(tlb, initialize_tlb)
{
    // srand(time(NULL));
    g_tlb = init_tlb();
    ASSERT_TRUE(g_tlb != NULL);
    ASSERT_TRUE(g_tlb->head == NULL);
    ASSERT_EQ(g_tlb->size, 0);
    free(g_tlb);
}

TEST(tlb, tlb_add_lookup)
{
    g_tlb = init_tlb();
    tlb_entry_t *enq = enqueue(g_tlb, 0x110, 0xABC);
    tlb_entry_t *enq1 = enqueue(g_tlb, 0x001, 0xBFC);
    tlb_entry_t *enq2 = enqueue(g_tlb, 0x101, 0xCDD);
    unsigned int lookup_num = 0x0011;
    tlb_entry_t *ret = look_up(g_tlb, lookup_num);
    ASSERT_EQ(g_tlb->head, enq);
    ASSERT_TRUE(g_tlb->head == enq);
    ASSERT_TRUE(get_page_num(g_tlb->head) == 0x110);
    ASSERT_TRUE(get_frame_addr(g_tlb->head) == 0xABC);
    ASSERT_TRUE(ret == NULL);
    ret = look_up(g_tlb, 0x001);
    ASSERT_TRUE(ret != NULL);
    ASSERT_TRUE(ret == enq1);
    ASSERT_TRUE(get_page_num(ret) == 0x001);
    ASSERT_TRUE(get_frame_addr(ret) == 0xBFC);
    free(g_tlb);
}

TEST(tlb, tlb_replacement)
{
    g_tlb = init_tlb();
    for (int i = 0; i < TLB_MAX_NUM_ENTRY; i++)
    {
        std::cout << i << std::endl; 
        enqueue(g_tlb, i * 4, i * 8);
    }
    ASSERT_EQ(g_tlb->size, TLB_MAX_NUM_ENTRY);
    ASSERT_TRUE(get_page_num(g_tlb->head) == 0x0);
    ASSERT_TRUE(get_frame_addr(g_tlb->head) == 0x0);

    tlb_entry_t *enq1 = enqueue(g_tlb, 0xFAAB, 0xBBBC);
    std::cout << "Size = " << g_tlb->size << std::endl;
    ASSERT_TRUE(g_tlb->size == TLB_MAX_NUM_ENTRY);
    ASSERT_TRUE(get_page_num(g_tlb->head) == 0x0004);
    ASSERT_TRUE(get_frame_addr(g_tlb->head) == 0x0008);

    tlb_entry_t *curr = g_tlb->head;
    while (curr->next)
    {
        curr = curr->next;
    }
    std::cout << "Last tlb entry's page number = " << get_page_num(curr) << std::endl;
    std::cout << "Last tlb entry's frame address = " << get_frame_addr(curr) << std::endl;

    ASSERT_TRUE(get_page_num(curr) == 0xFAAB);
    ASSERT_TRUE(get_frame_addr(curr) == 0xBBBC);
    free(g_tlb);
}

TEST(tlb, tlb_dequeue_drop)
{
    g_tlb = init_tlb();
    for (int i = 0; i < TLB_MAX_NUM_ENTRY; i++)
    {
        std::cout << i << std::endl; 
        enqueue(g_tlb, i * 4, i * 8);
    }
    ASSERT_TRUE(g_tlb->size == TLB_MAX_NUM_ENTRY);
    tlb_entry_t *deq1 = dequeue(g_tlb);
    ASSERT_TRUE(get_page_num(deq1) == 0x0);
    tlb_entry_t *deq2 = dequeue(g_tlb);
    ASSERT_TRUE(get_page_num(deq2) == 0x4);
    ASSERT_TRUE(g_tlb->size == (TLB_MAX_NUM_ENTRY - 2));
    ASSERT_TRUE(get_page_num(g_tlb->head) == 0x8);
}