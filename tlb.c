//
// Created by ahmet on 2022-04-09.
//

#include "tlb.h"

tlb_t *init_tlb(tlb_t *tlb)
{
    tlb_t *in_func;
    if (!tlb)
        in_func = (tlb_t *) malloc(sizeof(tlb_t));
    in_func->size = 0;
    in_func->head = NULL;
    g_tlb = in_func;
    return in_func;
}

tlb_entry_t *enqueue(tlb_t *tlb, unsigned int page_num, unsigned int frame_addr)
{
    if (tlb->size > TLB_MAX_NUM_ENTRY)
        return NULL;
    tlb_entry_t *entry = (tlb_entry_t *) malloc(sizeof(tlb_entry_t));
    entry->data = ((page_num << 0x10) & MASK_PAGE_NUM) | (frame_addr & MASK_FRAME_ADDR);
    entry->next = NULL;
    tlb_entry_t *curr_tlb_entr = tlb->head;
    if (!curr_tlb_entr) {
        tlb->head = entry;
        tlb->size++;
        return entry;
    }
    while (curr_tlb_entr && curr_tlb_entr->next) {
        curr_tlb_entr = curr_tlb_entr->next;
    }
    curr_tlb_entr->next = entry;
    tlb->size++;
    return entry;

}

tlb_entry_t *dequeue(tlb_t *tlb)
{
    if (tlb->size <= 0)
        return NULL;

    if (tlb->size == 1) {
        tlb_entry_t *curr = tlb->head;
        tlb->size--;
        tlb->head = NULL;
        return curr;
    }

    tlb_entry_t *tmp_head = tlb->head;
    tlb_entry_t *after_head = tmp_head->next;
    tlb->head = after_head;
    tlb->size--;
    return tmp_head;
}


tlb_entry_t *look_up(tlb_t *tlb, unsigned int page_num)
{
    tlb_entry_t *curr = tlb->head;
    while (curr && curr->next)
    {
        if (get_page_num(curr) == page_num)
            return curr;
        curr = curr->next;
    }
    return  NULL;
}

unsigned int get_frame_addr(tlb_entry_t *entry)
{
    return (entry->data & MASK_FRAME_ADDR);
}

unsigned int get_page_num(tlb_entry_t *entry)
{
    return ((entry->data & MASK_PAGE_NUM) >> 16);
}
