#include "buddy.h"
#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

struct {
    struct ORDERLIST orderList[20];
    int maxOrder;
    void *vstart;

    struct spinlock buddyLock;
    int use_lock;
} kmemAlloc;

struct NODELIST *freeNodeList;
int nodePageCount = 0;
int nodeListPageCount = 0;
void *nodeStart;
void *nodeListStart;


// ORIGINAL //

void bfreerange(void *vstart, void *vend) {
    char *p;
    p = (char*)PGROUNDUP((uint)vstart);
    void *startAddress = (void *)p;

    int pageCount = 0;
    for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
        //kfree(p);
        pageCount++;

    int tempPageCount = pageCount, totalPage = 1;
    while (tempPageCount >> 1) {
        tempPageCount = tempPageCount >> 1;
        totalPage *= 2;
    }

    nodePageCount = (totalPage * sizeof(struct NODE)) / PGSIZE;
    if ((totalPage * sizeof(struct NODE)) % PGSIZE != 0) nodePageCount++;
    nodeListPageCount = (totalPage * sizeof(struct NODELIST)) / PGSIZE;
    if ((totalPage * sizeof(struct NODE *)) % PGSIZE != 0) nodeListPageCount++;

    nodeStart = startAddress;
    nodeListStart = nodeStart + nodePageCount * PGSIZE;
    kmemAlloc.vstart = nodeListStart + nodeListPageCount * PGSIZE;

    int order = 0;
    pageCount = pageCount - nodePageCount - nodeListPageCount;
    while (pageCount >> 1) {
        pageCount = pageCount >> 1;
        order++;
    }

    kmemAlloc.maxOrder = order;
}

void bkfree(char *v) {
    if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
        panic("kfree");

    memset(v, 0, PGSIZE);
}


void buddyInit(void *vstart, void *vend) {
    initlock(&kmemAlloc.buddyLock, "kmemAlloc");
    kmemAlloc.use_lock = 0;

    bfreerange(vstart, vend);
    linkNodeList();

    kmemAlloc.orderList[kmemAlloc.maxOrder].freeList = (struct FREELIST *)vstart;
    kmemAlloc.use_lock = 1;
}

char *dmalloc(int size) {
    if (size < PGSIZE)
        panic("dmalloc size should be at least PGSIZE");

    if (kmemAlloc.use_lock)
        acquire(&kmemAlloc.buddyLock);

    int targetOrder = calOrder(size);
    int allocOrder = targetOrder;

    while(allocOrder <= kmemAlloc.maxOrder) {
        if (kmemAlloc.orderList[allocOrder].freeList) {
            char *ret = divideOrder(targetOrder, allocOrder);
            addSize((void *)ret, size);

            if (kmemAlloc.use_lock)
                release(&kmemAlloc.buddyLock);

            return ret;
        }

        allocOrder++;
    }

    if (kmemAlloc.use_lock)
        release(&kmemAlloc.buddyLock);

    panic("out of memory");
}

int calOrder(int size) {
    int orderSize = PGSIZE;
    int order = 0;

    while(orderSize < size) {
        orderSize *= 2;
        order++;
    }

    return order;
}

char *divideOrder(int targetOrder, int allocOrder) {
    if (targetOrder == allocOrder) {
        struct FREELIST *ret = kmemAlloc.orderList[allocOrder].freeList;
        kmemAlloc.orderList[allocOrder].freeList = ret->next;

        return (char *)ret;
    }

    struct FREELIST *chunk = kmemAlloc.orderList[allocOrder].freeList;
    kmemAlloc.orderList[allocOrder].freeList = chunk->next;

    int cnt = allocOrder, space = 1;
    while(--cnt) space *= 2;

    char *address = (char *)chunk + (space * PGSIZE);
    ((struct FREELIST *)(address))->next = kmemAlloc.orderList[allocOrder - 1].freeList;
    kmemAlloc.orderList[allocOrder - 1].freeList = ((struct FREELIST *)(address));

    chunk->next = kmemAlloc.orderList[allocOrder - 1].freeList;
    kmemAlloc.orderList[allocOrder - 1].freeList = chunk;

    return divideOrder(targetOrder, allocOrder - 1);
}

void printBuddySystem() {
    cprintf("printBuddySystem call\n");
    for (int i = 6;i >= 0;i--) {
        cprintf("order %d =>   ", i);

        struct FREELIST *r = kmemAlloc.orderList[i].freeList;
        while(r) {
            cprintf("%x - ", r);
            r = r->next;
        }
        cprintf("\n\n");
    }
}


void linkNodeList() {
    char *node;
    char *nodeList;
    node = (char *)nodeStart;
    nodeList = (char *)nodeListStart;

    struct NODELIST *r;
    for (int i = 0;i < (nodePageCount * PGSIZE) / sizeof(struct NODE);i++) {
        r = (struct NODELIST *)nodeList;
        r->address = (struct NODE *)node;
        r->next = freeNodeList;
        freeNodeList = r;

        node += sizeof(struct NODE);
        nodeList += sizeof(struct NODELIST);
    }
}

struct NODE *createNode(void *key, int value) {
    struct NODE *newNode;
    newNode = freeNodeList->address;
    newNode->list = freeNodeList;
    freeNodeList = freeNodeList->next;

    newNode->key = key;
    newNode->value = value;
    newNode->next = 0;

    return newNode;
}

int hashFunc(void *key) {
    return (uint)key % HASHSIZE;
}

void addSize(void *start, int size) {
    struct NODE *newNode = createNode(start, size);
    int hashIndex = hashFunc(start);
    
    if (bucket[hashIndex].head) {
        newNode->next = bucket[hashIndex].head;
        bucket[hashIndex].head = newNode;
    }
    else bucket[hashIndex].head = newNode;
}

int removeSize(void *start) {
    int hashIndex = hashFunc(start);
    struct NODE *deleteNode;
    struct NODE *prevNode;

    deleteNode = bucket[hashIndex].head;
    prevNode = bucket[hashIndex].head;
    while(deleteNode) {
        if (deleteNode->key == start) {
            if (prevNode == deleteNode) {
                bucket[hashIndex].head = deleteNode->next;
            }
            else {
                prevNode->next = deleteNode->next;
            }

            deleteNode->list->next = freeNodeList;
            freeNodeList = deleteNode->list;

            return deleteNode->value;
        }

        prevNode = deleteNode;
        deleteNode = deleteNode->next;
    }

    return -1;
}

void printHash() {
    cprintf("printHash call\n");
    for (int i = 0;i < HASHSIZE;i++) {
        cprintf("HASH[%d] =>   ", i);

        struct NODE *r = bucket[i].head;
        while(r) {
            cprintf("(%x , %d) - ", r->key, r->value);
            r = r->next;
        }
        cprintf("\n\n");
    }
}

void dfree(char *address) {
    int size;
    size = removeSize((void *)address);

    if (size <= 0) panic("dfree");

    if (kmemAlloc.use_lock)
        acquire(&kmemAlloc.buddyLock);

    int order = calOrder(size);
    struct FREELIST *freeList;
    freeList = (struct FREELIST *)address;
    freeList->next = kmemAlloc.orderList[order].freeList;
    kmemAlloc.orderList[order].freeList = freeList;

    mergeOrder(order);

    if (kmemAlloc.use_lock)
        release(&kmemAlloc.buddyLock);

    return;
}

void mergeOrder(int targetOrder) {
    struct FREELIST *targetList;
    targetList = kmemAlloc.orderList[targetOrder].freeList;

    struct FREELIST *candidate;
    struct FREELIST *prevList;
    candidate = kmemAlloc.orderList[targetOrder].freeList->next;
    prevList = candidate;

    int space = PGSIZE, order = targetOrder;
    while(order >= 0) {
        space *= 2;
        order--;
    }

    char *buddy, *mergeAddress;
    if ((int)targetList % space == 0) {
        buddy = (char *)targetList + (space / 2);
        mergeAddress = (char *)targetList;
    }

    else {
        buddy = (char *)targetList - (space / 2);
        mergeAddress = buddy;
    }
    cprintf("buddy : %x, merge : %x\n", buddy, mergeAddress);

    while(candidate) {
        if ((char *)candidate == buddy) {
            kmemAlloc.orderList[targetOrder].freeList = targetList->next;

            if (prevList == candidate) {
                kmemAlloc.orderList[targetOrder].freeList = candidate->next;
            }
            else {
                prevList->next = candidate->next;
            }

            struct FREELIST *mergeList;
            mergeList = (struct FREELIST *)mergeAddress;
            mergeList->next = kmemAlloc.orderList[targetOrder + 1].freeList;
            kmemAlloc.orderList[targetOrder + 1].freeList = mergeList;

            mergeOrder(targetOrder + 1);
            break;
        }

        prevList = candidate;
        candidate = candidate->next;
    }

    return;
}