#ifndef SLAB_H
#define SLAB_H

#include "types.h"

#define SLAB_NUM 10
#define MAX_FREE_PAGE 2
#define HASH_SIZE 3

struct page{
  struct page* next;
  struct page* prev;
  uint in_use_num;

  char* page_start;
  uint* free_list_head;
  uint* free_list_tail;
};

struct slab_cache{
  uint obj_sz;
  uint free_page_num;

  struct page static_page;

  struct page* cur_slab_page;
  struct page* page_tail;
};

struct hash_node{
  uint slab_idx;
  struct page* cur_page;
  struct hash_node* next;
  struct hash_node* prev;
};

struct hash_ptr{
  struct hash_node* head;
  struct hash_node* tail;
};

enum INSERT_FLAG{
  HEAD,
  TAIL
};

enum FREE_STATUS{
  NOT_FOUND,
  INVALID,
  SUCCESS
};

void make_slab_obj(struct page* cur_page, uint obj_sz);
struct page alloc_new_slab_page(uint slab_idx);
struct hash_node* find_target_page(void* slab_obj);
uint chech_slab_obj(uint obj_addr, uint obj_sz);
int free_slab_obj(void* slab_obj);
void free_slab_page(uint slab_idx, struct page* slab_page);

uint calc_hash_idx(uint page_addr);
void insert_hash_node(uint slab_idx, struct page* slab_page);
void return_hash_node(uint hash_idx, struct hash_node* tar_node);

void insert_slab_page(uint flag, uint slab_idx, struct page* slab_page);
void return_slab_page(uint slab_idx, struct page* slab_page);
int push_free_list(struct page* cur_page, void* slab_obj);
void* pop_free_list(uint slab_idx);

void slab_init(void);
void slab_test(void);

void print_slab(uint slab_idx);
void print_hash(void);

uint calc_slab_idx(uint N);
void load_next_slab_page(uint slab_idx);
struct page* alloc_new_page_addr(uint page_size);
void* slab_system(uint N);
void* kmalloc(uint N);

#endif