#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

char filename[8][8]={"README","README2","README3","README4","README5","README6","README7"};

char *p[8];
int v[8];
/*
int main(int argc, char* argv[])
{

    for (int i = 0; i < 3; i++){
        printf(1,"ith readme %d\n",i+1);
        p[i] = malloc(4096*3);
        v[i] = caching_open(filename[i],O_RDWR);
        //read(v[i],p[i],4096*3);
        //printf(1,"\n\n");
    }
    int pid = fork();
    if(pid > 0){
        caching_read(v[0],p[0],20,0);
         
        for (int i = 0; i < 3; i++){
            caching_write(v[i],"AAAAA",5,8126);
        }
        //printf(1,"|||||||||||||||||||||||||||||||||||||||||||||||||||AFTER CLOSE\n");
        for (int i = 0; i < 3; i++) caching_close(v[i]);
        

        //printf(1,"OPEN REPEAT\n");
        for (int i = 0; i < 3; i++){
            v[i] = caching_open(filename[i],O_RDWR);
            caching_read(v[i],p[i],4096*3,0);
        } 
        
        printf(1,"WRITE DIRECT READ\n");
        printf(1,"README CONTENTS 20 BYTES AFTER WRITE1\n");
        for (int i = 8120; i < 8120+20; i++){
            printf(1,"%c ",p[0][i]);
        }
        printf(1,"\n\n");
        printf(1,"README2 CONTENTS 20 BYTES AFTER WRITE1\n");
        for (int i = 8120; i < 8120+20; i++){
            printf(1,"%c ",p[1][i]);
        }
        printf(1,"\n\n"); 
 
        printf(1,"README3 CONTENTS 20 BYTES AFTER WRITE1\n");
        for (int i = 8120; i < 8120+20; i++){
            printf(1,"%c ",p[2][i]);
        }
        printf(1,"\n\n"); 

    }
    else if (pid == 0){
        caching_read(v[1],p[1],20,8120);
        caching_write(v[1],"BBBB",4,3);
        caching_read(v[1],p[1],20,0);
        for (int i = 0; i < 20 ;i++){
            printf(1,"%c ",p[1][i]);
        }
        printf(1,"\n");
        exit();
        
    }


    wait();


    for (int i = 0; i < 3; i++) caching_close(v[i]);
    printf(1,"AFTER CLOSE\n");

    printf(1,"OPEN REPEAT\n");
    for (int i = 0; i < 3; i++){
        v[i] = caching_open(filename[i],O_RDWR);
        caching_read(v[i],p[i],4096*3,0);
    }    
    
    printf(1,"WRITE DIRECT READ\n");


    printf(1,"README CONTENTS 20 BYTES AFTER WRITE1\n");
    for (int i = 4090; i < 4090+20; i++){
        printf(1,"%c ",p[0][i]);
    }
    printf(1,"\n\n");
    printf(1,"README2 CONTENTS 20 BYTES AFTER WRITE1\n");
    for (int i = 0; i < 20; i++){
        printf(1,"%c ",p[1][i]);
    }
    printf(1,"\n\n"); 

    printf(1,"README2 CONTENTS 20 BYTES AFTER WRITE2\n");
    for (int i = 4090; i < 4090+20; i++){
        printf(1,"%c ",p[1][i]);
    }
    printf(1,"\n\n");    
    printf(1,"README3 CONTENTS 20 BYTES AFTER WRITE1\n");
    for (int i = 4090; i < 4090+20; i++){
        printf(1,"%c ",p[2][i]);
    }
    printf(1,"\n\n"); 
    exit();

}*/

/*
int main(int argc, char* argv[])
{
    int v= caching_open("README",O_RDWR);
    char *p=malloc(4096*3);
    int pid = fork();

    if(pid  > 0) {
        char *name = "cccccc";
        caching_write(v,name,6,0);
    }
    else if(pid == 0){
        char *name = "dddddd";
        caching_write(v,name,6,0);

        exit();
    }
    else if (pid < 0) {
        printf(1,"fork fail\n");
    }

    wait();

    printf(1,"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
    caching_read(v,p,4096,0);

    for (int i = 0; i < 20; i++){
        printf(1,"%c",p[i]);
    }
    printf(1,"\n\n");    
    caching_read(v,p,4096,4090);
    for (int i = 0; i < 20; i++){
        printf(1,"%c",p[i]);
    }
    printf(1,"\n\n");
    //close(v);
    exit();
}*/

/*int main(int arge, char * argv[])
{
    int v= open("README",O_RDWR);
    char *p=malloc(4096*3);
    char *l = "hello";
    write(v,l,5);
    fileoffset(v,0);
    read(v,p,4096);

    for (int i = 0; i < 20; i++){
        printf(1,"%c",p[i]);
    }
    printf(1,"\n\n");
    exit();
}*/


int
main(int argc, char* argv[])
{   
    nvme_setting();
    //printf(1,"pid is %d\n",getpid());
    /*cd("|");
    printf(1, "change cwd????\n");
    int tmp1 = get_ticks();
    
    int V = open("README",O_RDWR);
    printf(1, "%d %d\n",tmp1,V);
    char *p = malloc(4096);

    
    fileoffset(V,4093);
    char *q="pumpk";
    write(V,q,5);
    fileoffset(V,4093);
    read(V,p,10);

    printf(1,"\nresult\n");
    for (int i = 0; i < 10; i++){
        printf(1,"%d ",p[i]);
    }
    printf(1,"\nEND\n");
    close(V);

    V = open("README",O_RDWR);
    fileoffset(V,4093);
    read(V,p,10);

    printf(1,"\nresult\n");
    for (int i = 0; i < 10; i++){
        printf(1,"%d ",p[i]);
    }
    printf(1,"\nEND\n");
    int tmp2 = get_ticks();
    printf(1,"-------------------------------------------------------\n");
    cd("/");
    V = open("README2",O_RDWR);
    fileoffset(V,4093);
    read(V,p,10);

    printf(1,"\nresult\n");
    for (int i = 0; i < 10; i++){
        printf(1,"%d ",p[i]);
    }
    printf(1,"\nEND\n");

    fileoffset(V,4093);
    write(V,q,5);
    fileoffset(V,4093);
    read(V,p,10);
    printf(1,"\nresult\n");
    for (int i = 0; i < 10; i++){
        printf(1,"%d ",p[i]);
    }
    printf(1,"\nEND\n");

    close(V);

    V = open("README2",O_RDWR);
    fileoffset(V,4093);
    read(V,p,10);

    printf(1,"\nresult\n");
    for (int i = 0; i < 10; i++){
        printf(1,"%d ",p[i]);
    }
    printf(1,"\nEND\n");

    int tmp3 = get_ticks();

    printf(1,"nvme %d disk %d\n",tmp2-tmp1, tmp3-tmp2);*/
    for (int i = 2; i < 7; i++){
        p[i] = malloc(4096*3);
    }    
    int tmp1 = get_ticks();
    //cd("|");
    for (int i = 2; i < 7; i++){
        v[i] = caching_open(filename[i],O_RDWR);
        //printf(1,"%d NOOOOOOOOOOO\n", v[i]);
        caching_read(v[i%7],p[i%7],4096*3,0);
    }   
    for (int k = 0; k < 100; k++){
        for (int i = 2; i < 7; i++){
            char *q= "pumpk";
            caching_write(v[i%7],q,5,4095);
        }
    } 
    for (int i = 2; i < 7; i++){
        caching_read(v[i%7],p[i%7],10,4093);
    }
    for (int i = 2; i < 7; i++) caching_close(v[i]);
    for (int i = 2; i < 7; i++){
        v[i] = caching_open(filename[i],O_RDWR);
        caching_read(v[i%7],p[i%7],4096*3,0);
    }
    int tmp2 = get_ticks();
////////////////////////////////////////////////////////////////////////////////////////////////////
    for (int i = 2; i < 7; i++){
        //p[i] = malloc(4096*3);
        v[i] = open(filename[i],O_RDWR);
        read(v[i],p[i],4096*3);
    }

    for(int k = 0;k < 100; k++){
    for (int i = 2; i < 7; i++){
        char *q= "pumpk";
        fileoffset(v[i%7],4095);
        write(v[i%7],q,5);

    }
    }

    for (int i = 2; i < 7; i++){
        fileoffset(v[i%7],4093);
        read(v[i%7],p[i%7],10);
    }

    for (int i = 2; i < 7; i++) close(v[i]);

    for (int i = 2; i < 7; i++){
        v[i] = open(filename[i],O_RDWR);
        fileoffset(v[i%7],0);
        read(v[i%7],p[i%7],4096*3);
    }
    int tmp3 = get_ticks();
    printf(1,"%d %d\n",tmp2-tmp1, tmp3-tmp2);


    exit();
}
