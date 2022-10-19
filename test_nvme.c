#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

/*
nvme 기본 테스트
파일 시스템 올린 상태로 read, write 진행

+

페이지 캐시 합치기



*/


char filename[8][8]={"README","README2","README3","README4","README5"};

char *p[8];
int v[8];



int
main(int argc, char* argv[])
{   
    for (int i = 0; i < 5; i++){
        p[i] = malloc(4096*3);
    }    
    int tmp1 = get_ticks();
    cd("|");
    for (int i = 0; i < 5; i++){
        v[i] = caching_open(filename[i],O_RDWR);
        //printf(1,"%d NOOOOOOOOOOO\n", v[i]);
        caching_read(v[i%7],p[i%7],4096*3,0);
    }   
    for (int k = 0; k < 100; k++){
        for (int i = 0; i < 5; i++){
            char *q= "pumpk";
            caching_write(v[i%7],q,5,4095);
        }
    } 
    for (int i = 0; i < 5; i++){
        caching_read(v[i%7],p[i%7],10,4093);
    }
    for (int i = 0; i < 5; i++) caching_close(v[i]);
    for (int i = 0; i < 5; i++){
        v[i] = caching_open(filename[i],O_RDWR);
        caching_read(v[i%7],p[i%7],4096*3,0);
    }
    for (int i = 0; i < 5; i++){
        for (int j = 0; j < 10; j++){
            printf(1,"%c ", p[i][4090 + j]);
        }
        printf(1,"\n");
    }
    int tmp2 = get_ticks();
////////////////////////////////////////////////////////////////////////////////////////////////////
    for (int i = 0; i < 5; i++){
        //p[i] = malloc(4096*3);
        v[i] = open(filename[i],O_RDWR);
        read(v[i],p[i],4096*3);
    }

    for(int k = 0;k < 100; k++){
    for (int i = 0; i < 5; i++){
        char *q= "ooooo";
        fileoffset(v[i%7],4095);
        write(v[i%7],q,5);

    }
    }

    for (int i = 0; i < 5; i++){
        fileoffset(v[i%7],4093);
        read(v[i%7],p[i%7],10);
    }

    for (int i = 0; i < 5; i++) close(v[i]);

    for (int i = 0; i < 5; i++){
        v[i] = open(filename[i],O_RDWR);
        fileoffset(v[i%7],0);
        read(v[i%7],p[i%7],4096*3);
    }
    for (int i = 0; i < 5; i++){
        for (int j = 0; j < 10; j++){
            printf(1,"%c ", p[i][4090 + j]);
        }
        printf(1,"\n");
    }    
    int tmp3 = get_ticks();
    printf(1,"%d %d\n",tmp2-tmp1, tmp3-tmp2);


    exit();
}
