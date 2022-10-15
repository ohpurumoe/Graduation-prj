#include "slab.h"
#include "buddy.h"
#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"

struct slab_cache slab[SLAB_NUM];
struct hash_ptr hash_table[HASH_SIZE];

void make_slab_obj(struct page* cur_page, uint obj_sz){
  uint cnt = 1;
  uint total_obj = PGSIZE / obj_sz;
  uint* cur_obj;

  for(cur_obj = (uint*)cur_page->page_start; cnt < total_obj; cnt++, cur_obj += (obj_sz / sizeof(uint))){
    *cur_obj = (uint)(cur_obj + (obj_sz / sizeof(uint)));
  }

  *cur_obj = 0;

  cur_page->in_use_num = 0;
  cur_page->free_list_head = (uint*)(cur_page->page_start);
  cur_page->free_list_tail = cur_obj;
}

struct page alloc_new_slab_page(uint slab_idx){
  struct page new_page;

  memset(&new_page, 0, sizeof(new_page));

  if((new_page.page_start = dmalloc(PGSIZE)) != 0){
     make_slab_obj(&new_page, slab[slab_idx].obj_sz);
     slab[slab_idx].free_page_num++;
  }

  return new_page;
}

struct hash_node* find_target_page(void* slab_obj){
  uint page_addr = (((uint)slab_obj) / PGSIZE) * PGSIZE;
  uint hash_idx = calc_hash_idx(page_addr);

  struct hash_node* cur_node = hash_table[hash_idx].head;

  while(cur_node){
    if(((uint)cur_node->cur_page->page_start) == page_addr) return cur_node;
    cur_node = cur_node->next;
  }

  return 0;
}

uint check_slab_obj(uint obj_addr, uint obj_sz){
  obj_addr %= PGSIZE;

  if(obj_addr % obj_sz != 0) return 0;
  else return 1;
}

int free_slab_obj(void* slab_obj){
  struct hash_node* hnode = find_target_page(slab_obj);

  if(hnode == 0) return NOT_FOUND;

  struct page* tar_page = hnode->cur_page;
  uint slab_idx = hnode->slab_idx;

  if(tar_page == 0) return INVALID;
  if(!check_slab_obj((uint)slab_obj, slab[slab_idx].obj_sz)) return INVALID;

  if(push_free_list(tar_page, slab_obj) == INVALID) return INVALID;

  if((tar_page->in_use_num) == 0){
    slab[slab_idx].free_page_num++;

    if(slab[slab_idx].free_page_num > MAX_FREE_PAGE){
      uint page_addr = (((uint)slab_obj) / PGSIZE) * PGSIZE;
      uint hash_idx = calc_hash_idx(page_addr);

      return_hash_node(hash_idx, hnode);
      return_slab_page(slab_idx, tar_page);
    }
  }

  return SUCCESS;
}

void free_slab_page(uint slab_idx, struct page* slab_page){
  dfree(slab_page->page_start);
  free_slab_obj(slab_page);
}

uint calc_hash_idx(uint page_addr){
  return (page_addr / PGSIZE) % HASH_SIZE;
}

void insert_hash_node(uint slab_idx, struct page* slab_page){
  uint hash_idx = calc_hash_idx((uint)(slab_page->page_start));

  struct hash_node* cur_node = (struct hash_node*)dmalloc(sizeof(struct hash_node));
  cur_node->cur_page = slab_page;
  cur_node->slab_idx = slab_idx;

  if(hash_table[hash_idx].tail == 0){
    hash_table[hash_idx].head = cur_node;
    hash_table[hash_idx].tail = cur_node;
    cur_node->prev = cur_node->next = 0;
  }
  else{
    hash_table[hash_idx].tail->next = cur_node;
    cur_node->next = 0;
    cur_node->prev = hash_table[hash_idx].tail;
    hash_table[hash_idx].tail = cur_node;
  }
}

void return_hash_node(uint hash_idx, struct hash_node* tar_node){
  if(tar_node->prev != 0){
    tar_node->prev->next = tar_node->next;
  }
  if(tar_node->next != 0){
    tar_node->next->prev = tar_node->prev;
  }

  if(hash_table[hash_idx].head == tar_node){
    hash_table[hash_idx].head = tar_node->next;
  }
  if(hash_table[hash_idx].tail == tar_node){
    hash_table[hash_idx].tail = tar_node->prev;
  }

  free_slab_obj(tar_node);
}

void insert_slab_page(uint flag, uint slab_idx, struct page* slab_page){
  if(slab[slab_idx].cur_slab_page == 0){
    slab[slab_idx].page_tail = slab_page;
    slab_page->prev = slab_page->next = 0;
    slab[slab_idx].cur_slab_page = slab_page;
  }
  else if(flag == HEAD){
    slab[slab_idx].cur_slab_page->prev = slab_page;
    slab_page->next = slab[slab_idx].cur_slab_page;
    slab_page->prev = 0;
    slab[slab_idx].cur_slab_page = slab_page;
  }
  else{
    slab[slab_idx].page_tail->next = slab_page;
    slab_page->next = 0;
    slab_page->prev = slab[slab_idx].page_tail;
    slab[slab_idx].page_tail = slab_page;
  }
}

void return_slab_page(uint slab_idx, struct page* slab_page){
  if(slab_page->prev != 0){
    slab_page->prev->next = slab_page->next;
  }
  if(slab_page->next != 0){
    slab_page->next->prev = slab_page->prev;
  }

  if(slab[slab_idx].cur_slab_page == slab_page){
    slab[slab_idx].cur_slab_page = slab_page->next;
  }
  if(slab[slab_idx].page_tail == slab_page){
    slab[slab_idx].page_tail = slab_page->prev;
  }

  if(&(slab[slab_idx].static_page) != slab_page){
    free_slab_page(slab_idx, slab_page);
  }
  else{
    memset(&(slab[slab_idx].static_page), 0, sizeof(slab[slab_idx].static_page));
  }

  slab[slab_idx].free_page_num--;
}

int push_free_list(struct page* cur_page, void* slab_obj){
  uint* cur_obj = cur_page->free_list_head;

  while(cur_obj){
    if((uint)cur_obj == (uint)slab_obj) return INVALID;
    cur_obj = (uint*)(*cur_obj);
  }

  if(cur_page->free_list_tail != 0){
    *(cur_page->free_list_tail) = (uint)slab_obj;
    cur_page->free_list_tail = (uint*)slab_obj;
    *(cur_page->free_list_tail) = 0;
  }
  else{
    cur_page->free_list_head = (uint*)slab_obj;
    cur_page->free_list_tail = (uint*)slab_obj;
    *(cur_page->free_list_head) = 0;
  }

   cur_page->in_use_num--;
   return SUCCESS;
}

void* pop_free_list(uint slab_idx){
  struct page* cur_page = slab[slab_idx].cur_slab_page;

  if(cur_page->in_use_num == 0) slab[slab_idx].free_page_num--;

  uint* cur_free_obj = cur_page->free_list_head;

  
  cur_page->free_list_head = (uint*)(*(cur_page->free_list_head));
  cur_page->in_use_num++;

  if(cur_page->free_list_head == 0){
    cur_page->free_list_tail = 0;
  }

  return (void*)cur_free_obj;
}

void slab_init(void){
  uint slab_idx = 0;

  memset(hash_table, 0, sizeof(hash_table));
  for(uint sz = 4; sz <= 2048; sz += sz, slab_idx++){
    slab[slab_idx].obj_sz = sz;
    slab[slab_idx].cur_slab_page = 0;

    slab[slab_idx].static_page = alloc_new_slab_page(slab_idx);
    insert_slab_page(TAIL, slab_idx, &(slab[slab_idx].static_page));
    slab[slab_idx].cur_slab_page = &(slab[slab_idx].static_page);
  }

  for(uint slab_idx = 0; slab_idx < SLAB_NUM; slab_idx++){
    insert_hash_node(slab_idx, slab[slab_idx].cur_slab_page);
  }
}

void print_slab(uint slab_idx){
  cprintf("\n-------slab %d--------\n", slab_idx);
  struct page* cur_page = slab[slab_idx].cur_slab_page;

  cprintf("free_page_num : %d\n", slab[slab_idx].free_page_num);
  cprintf("cur_slab_page : %x in_use : %d ->\n", cur_page, cur_page->in_use_num);
  cur_page = cur_page->next;

  while(cur_page){
    cprintf("next page : %x in_use : %d ->\n", cur_page, cur_page->in_use_num);
    cur_page = cur_page->next;
  }

  cprintf("-----------------------\n");
}

void print_hash(void){
  cprintf("\n-------hash table--------\n");
  for(uint hash_idx = 0; hash_idx < HASH_SIZE; hash_idx++){
    cprintf("hash idx[%d] : ", hash_idx);
    struct hash_node* cur_node = hash_table[hash_idx].head;

    while(cur_node){
      cprintf("%x -> ", cur_node->cur_page);
      cur_node = cur_node->next;
    }

    cprintf("\n");
  }
  cprintf("--------------------------\n");
}

int calc_slab_mem(void){
  int npage = 0, nhash_node = 0;
  int slab_mem = SLAB_NUM * (sizeof(struct slab_cache)) + sizeof(struct hash_ptr);
  // (10 initial slab caches) + struct hash_ptr

  for(int i=0; i<SLAB_NUM; i++){
    struct page* cur_page = slab[i].cur_slab_page;
    struct page* static_page = &(slab[i].static_page);

    if((cur_page != static_page) && (cur_page != 0)) npage++;

    while(cur_page){
      cur_page = cur_page->next;
      if((cur_page != static_page) && (cur_page != 0)) npage++;
    }
  }

  for(int hash_idx = 0; hash_idx < HASH_SIZE; hash_idx++){
    struct hash_node* cur_node = hash_table[hash_idx].head;

    if(cur_node != 0) nhash_node++;

    while(cur_node){
      cur_node = cur_node->next;
      if(cur_node != 0) nhash_node++;
    }
  }

  slab_mem += (slab[calc_slab_idx(sizeof(struct page))].obj_sz) * npage ; // using slab per page
  slab_mem += (slab[calc_slab_idx(sizeof(struct hash_node))].obj_sz) * nhash_node; // using slab per hash node

  return slab_mem;
}

uint calc_slab_idx(uint N){
  uint power2 = 4, cnt = 0;

  if(N < 4) return 0;

  while(N > power2){
    power2 *= 2; cnt++;
  }

  return cnt;
}

void load_next_slab_page(uint slab_idx){
  struct page* cur_page = slab[slab_idx].cur_slab_page;
  struct page* next_page = cur_page->next;

  next_page->prev = 0;
  insert_slab_page(TAIL, slab_idx, cur_page);
  slab[slab_idx].cur_slab_page = next_page;
}

struct page* alloc_new_page_addr(uint page_size){
  uint slab_idx = calc_slab_idx(page_size);
  struct page* cur_page;
  struct page* prev_tail = slab[slab_idx].page_tail;

page_try_alloc:
  cur_page = slab[slab_idx].cur_slab_page;

  if(cur_page->free_list_head != 0){
    return pop_free_list(slab_idx);
  }
  else{
    goto page_no_free_obj;
  }

page_no_free_obj:
  if((cur_page->next != 0) && (cur_page != prev_tail)){
    load_next_slab_page(slab_idx);
    goto page_try_alloc;
  }
  else{
    struct page* new_page_addr;
    struct page new_page = alloc_new_slab_page(slab_idx);
    if(new_page.page_start == 0) goto page_alloc_fail;

    new_page_addr = (struct page*)(new_page.free_list_head);
    new_page.free_list_head = (uint*)(*(new_page.free_list_head));
    new_page.in_use_num++;
    slab[slab_idx].free_page_num--;

    *new_page_addr = new_page;
    insert_slab_page(HEAD, slab_idx, new_page_addr);
    insert_hash_node(slab_idx, new_page_addr);

    goto page_try_alloc;
  }

page_alloc_fail:
  return 0;
}

void* slab_system(uint N){
  uint slab_idx = calc_slab_idx(N);

  struct page* cur_page;
  struct page* prev_tail = slab[slab_idx].page_tail;
  struct page* new_page_addr;
  struct page new_page;

try_alloc:
  cur_page = slab[slab_idx].cur_slab_page;

  if(cur_page->free_list_head != 0){
    return pop_free_list(slab_idx);
  }
  else{
    goto no_free_obj;
  }

no_free_obj:
  if((cur_page->next != 0) && (cur_page != prev_tail)){
    load_next_slab_page(slab_idx);
    goto try_alloc;
  }
  else{
    new_page = alloc_new_slab_page(slab_idx);
    if(new_page.page_start == 0) {
      cprintf("fail alloc new slab page\n");
      goto alloc_fail;
    }

    if((new_page_addr = alloc_new_page_addr(sizeof(struct page))) != 0){
      *new_page_addr = new_page;
      
      insert_slab_page(HEAD, slab_idx, new_page_addr);
      insert_hash_node(slab_idx, new_page_addr);
      goto try_alloc;
    }
    else{
      cprintf("fail alloc new page addr\n");
      goto alloc_fail;
    }
  }

alloc_fail:
  if(new_page.page_start != 0){
    dfree(new_page.page_start);
  }
  return 0;
}

typedef struct {
  int a, b;
} TEST_TYPE;

TEST_TYPE* test_mem[(PGSIZE / sizeof(TEST_TYPE)) * 10];

void slab_test(void){
    cprintf("test start...\n");

    uint slab_idx = calc_slab_idx(sizeof(TEST_TYPE));
    uint test_unit = (PGSIZE / slab[slab_idx].obj_sz);

    print_slab(slab_idx);
    print_hash();

    for(uint i=0; i < (10 * test_unit); i++){
        TEST_TYPE* testptr = (TEST_TYPE*)dmalloc(sizeof(TEST_TYPE));

         if(testptr == 0) {
            cprintf("NULL PTR\n");
            goto fail;
        }

        test_mem[i] = testptr;
        testptr->a = i % PGSIZE;

        if(testptr->a != (i % PGSIZE)) {
            cprintf("assert : write %x & read %x\n", i % PGSIZE, testptr->a);
            goto fail;
        }
        
        //cprintf("testptr : %x\n", testptr);
        //free_slab_obj(testptr);
    }
    print_slab(slab_idx);
    print_hash();

    for(uint i = 5 * test_unit; i < (10 * test_unit); i++){
      dfree(test_mem[i]);
    }

    for(uint i = 0; i < (5 * test_unit); i++){
      dfree(test_mem[i]);
    }

    print_slab(slab_idx);
    print_hash();

    for(uint i=0; i < (10 * test_unit); i++){
        TEST_TYPE* testptr = (TEST_TYPE*)dmalloc(sizeof(TEST_TYPE));

         if(testptr == 0) {
            cprintf("NULL PTR\n");
            goto fail;
        }

        test_mem[i] = testptr;
        testptr->a = i % PGSIZE;

        if(testptr->a != (i % PGSIZE)) {
            cprintf("assert : write %x & read %x\n", i % PGSIZE, testptr->a);
            goto fail;
        }
        
        //cprintf("testptr : %x\n", testptr);
        //free_slab_obj(testptr);
    }

    for(uint i=0; i < (10 * test_unit); i++){
        if(test_mem[i]->a != (i % PGSIZE)) {
            cprintf("assert : write %x & read %x\n", i % PGSIZE, *(test_mem[i]));
            goto fail;
        }
        
        //cprintf("testptr : %x\n", testptr);
        //free_slab_obj(testptr);
    }

    print_slab(slab_idx);
    print_hash();

    cprintf("test success!\n");

    return;

fail:
    cprintf("test fail\n");
}