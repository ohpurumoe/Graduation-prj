//#define HASHSIZE    32771
#define HASHSIZE    3

struct FREELIST {
    struct FREELIST *next;
};

struct ORDERLIST {
    struct FREELIST *freeList;
};

struct NODE {
    void *key;
    int value;
    struct NODE *next;
    struct NODELIST *list;
};

struct NODELIST {
    struct NODE *address;
    struct NODELIST *next;
};

struct HASHTABLE {
    struct NODE *head;
};
struct HASHTABLE bucket[HASHSIZE];

extern char end[];

void buddyInit(void *vstart, void *vend);
char *dmalloc(int size);
int calOrder(int size);
char *divideOrder(int targetOrder, int allocOrder);
void printBuddySystem();

void linkNodeList();
struct NODE *createNode(void *key, int value);
int hashFunc(void *key);
void addSize(void *start, int size);
int removeSize(void *start);
void printHash();

void dfree(void *address);
void mergeOrder(int targetOrder);