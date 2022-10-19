#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int
main(int argc, char* argv[])
{   
    char num[5];
    int cnt = 0, req_c = 0, req_m = 0, total_mem = 0, kalloc_mem = 0;

    int fd = open("mem_req.txt", O_RDONLY);
    
    while(1){
        char n;

        if(read(fd, &n, 1) == 0) break;
        
        if(n >= '0' && n <= '9'){
            num[cnt++] = n;
        }
        else {
            num[cnt] = 0; cnt = 0; req_m = atoi(num);
            kalloc_mem += 4096; total_mem = kernel_dmalloc(req_m);

            if(total_mem != -1) printf(1, "req %d: %d %d %d\n", req_c++, req_m, kalloc_mem, total_mem);
            else goto fail;
        }
    }

    printf(1, "slab test success!\nkalloc_mem : %d total_mem : %d\n", kalloc_mem, total_mem);

    printf(1,"buddy test begin!\n");
    buddy_testing();

    exit();

fail:
    printf(1, "slab test failed\n");
    printf(1,"buddy test begin!\n");
    buddy_testing();
    exit();
}