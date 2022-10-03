#include "slab.h"
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

  if((new_page.page_start = kalloc()) != 0){
     make_slab_obj(&new_page, slab[slab_idx].obj_sz);
     slab[slab_idx].free_page_num++;
  }

  return new_page;
}

struct hash_node* find_target_page(uint hash_idx, uint page_addr){
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

void free_slab_obj(void* slab_obj){
  uint page_addr = (((uint)slab_obj) / PGSIZE) * PGSIZE;
  uint hash_idx = calc_hash_idx(page_addr);

  struct hash_node* hnode = find_target_page(hash_idx, page_addr);

  if(hnode == 0) return;

  struct page* tar_page = hnode->cur_page;
  uint slab_idx = hnode->slab_idx;

  if(!check_slab_obj((uint)slab_obj, slab[slab_idx].obj_sz)) return;

  //cprintf("slab_idx : %d hnode : %x\n", slab_idx, hnode);

  if(tar_page == 0) return;

  push_free_list(tar_page, slab_obj);

  if((tar_page->in_use_num) == 0){
    slab[slab_idx].free_page_num++;

    if(slab[slab_idx].free_page_num > MAX_FREE_PAGE){
      return_slab_page(slab_idx, tar_page);
      return_hash_node(hash_idx, hnode);
    }
  }
}

void free_slab_page(uint slab_idx, struct page* slab_page){
  kfree(slab_page->page_start);
  free_slab_obj(slab_page);
}

uint calc_hash_idx(uint page_addr){
  return (page_addr / PGSIZE) % HASH_SIZE;
}

void insert_hash_node(uint slab_idx, struct page* slab_page){
  uint hash_idx = calc_hash_idx((uint)(slab_page->page_start));

  struct hash_node* cur_node = (struct hash_node*)kmalloc(sizeof(struct hash_node));
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

void push_free_list(struct page* cur_page, void* slab_obj){
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

uint calc_slab_idx(uint N){
  uint power2 = 4, cnt = 0;

  if(N < 4) return 0;

  while(N > power2){
    if(N == 48) cprintf("power2 : %d cnt : %d\n", power2, cnt);
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
    //cprintf("load next page obj page : %x\n", slab[slab_idx].cur_slab_page);
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

    //cprintf("alloc page obj page: %x\n", new_page_addr);
    goto page_try_alloc;
  }

page_alloc_fail:
  return 0;
}

void* slab_system(uint N){
  uint slab_idx = calc_slab_idx(N);
  struct page* cur_page;
  struct page* prev_tail = slab[slab_idx].page_tail;

try_alloc:
  cur_page = slab[slab_idx].cur_slab_page;

  if(cur_page->free_list_head != 0){
    return pop_free_list(slab_idx);
  }
  else{
    //cprintf("\n-------slab_system call------\n");
    goto no_free_obj;
  }

no_free_obj:
  if((cur_page->next != 0) && (cur_page != prev_tail)){
    load_next_slab_page(slab_idx);
    //cprintf("load next slab page : %x\n", slab[slab_idx].cur_slab_page);
    goto try_alloc;
  }
  else{
    struct page* new_page_addr;
    struct page new_page = alloc_new_slab_page(slab_idx);
    if(new_page.page_start == 0) {
      cprintf("fail alloc new slab page\n");
      goto alloc_fail;
    }

    if((new_page_addr = alloc_new_page_addr(sizeof(struct page))) != 0){
      *new_page_addr = new_page;
      
      insert_slab_page(HEAD, slab_idx, new_page_addr);
      insert_hash_node(slab_idx, new_page_addr);

      //cprintf("alloc new slab page : %x\n", slab[slab_idx].cur_slab_page);
      goto try_alloc;
    }
    else{
      cprintf("fail alloc new page addr\n");
      goto alloc_fail;
    }
  }

alloc_fail:
  return 0;
}

void* kmalloc(uint N){
  if((N == 0) || (N > PGSIZE)) return 0;

  return slab_system(N);
}

typedef struct hash_node TEST_TYPE;
TEST_TYPE* test_mem[(PGSIZE / sizeof(TEST_TYPE)) * 10];

void slab_test(void){
    cprintf("test start...\n");

    uint slab_idx = calc_slab_idx(sizeof(TEST_TYPE));
    uint test_unit = (PGSIZE / slab[slab_idx].obj_sz);

    print_slab(slab_idx);
    print_hash();

    for(uint i=0; i < (3 * test_unit); i++){
        TEST_TYPE* testptr = (TEST_TYPE*)kmalloc(sizeof(TEST_TYPE));

         if(testptr == 0) {
            cprintf("NULL PTR\n");
            goto fail;
        }

        test_mem[i] = testptr;
        testptr->slab_idx = i % PGSIZE;

        if(testptr->slab_idx != (i % PGSIZE)) {
            cprintf("assert : write %x & read %x\n", i % PGSIZE, testptr->slab_idx);
            goto fail;
        }
        
        //cprintf("testptr : %x\n", testptr);
        //free_slab_obj(testptr);
    }
    print_slab(slab_idx);
    print_hash();

    for(uint i = test_unit; i < (3 * test_unit); i++){
      free_slab_obj(test_mem[i]);
    }

    for(uint i = 0; i < (test_unit); i++){
      free_slab_obj(test_mem[i]);
    }

    print_slab(slab_idx);
    print_hash();

    for(uint i=0; i < (3 * test_unit); i++){
        TEST_TYPE* testptr = (TEST_TYPE*)kmalloc(sizeof(TEST_TYPE));

         if(testptr == 0) {
            cprintf("NULL PTR\n");
            goto fail;
        }

        test_mem[i] = testptr;
        testptr->slab_idx = i % PGSIZE;

        if(testptr->slab_idx != (i % PGSIZE)) {
            cprintf("assert : write %x & read %x\n", i % PGSIZE, testptr->slab_idx);
            goto fail;
        }
        
        //cprintf("testptr : %x\n", testptr);
        //free_slab_obj(testptr);
    }

    for(uint i=0; i < (3 * test_unit); i++){
        if(test_mem[i]->slab_idx != (i % PGSIZE)) {
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