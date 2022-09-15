#ifndef SLAB_H
#define SLAB_H

#include "types.h"

#define SLAB_NUM 10
#define MAX_FREE_PAGE 2

struct page{
  struct page* next;
  struct page* prev;
  uint in_use_num;

  char* page_start;
  uint* free_list_head;
  uint* free_list_tail;
};

struct page_free{
  struct page* cur_page;
  uint slab_idx;
};

enum INSERT_FLAG{
  HEAD,
  TAIL
};

void make_slab_obj(struct page* cur_page, uint obj_sz);
struct page alloc_new_slab_page(uint slab_idx);
struct page_free find_target_page(uint page_addr);
void free_slab_obj(void* slab_obj);
void free_slab_page(uint slab_idx, struct page* slab_page);
void insert_slab_page(uint flag, uint slab_idx, struct page* slab_page);
void return_slab_page(uint slab_idx, struct page* slab_page);
void push_free_list(struct page* cur_page, void* slab_obj);
void* pop_free_list(uint slab_idx);
void slab_init(void);
void slab_test(void);
void print_slab(uint slab_idx);
uint calc_slab_idx(uint N);
void load_next_slab_page(uint slab_idx);
struct page* alloc_new_page_addr(uint page_size);
void* slab_system(uint N);
void* kmalloc(uint N);

#endif