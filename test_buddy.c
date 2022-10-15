#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

/*
nvme 기본 테스트
파일 시스템 올린 상태로 read, write 진행
*/


  printBuddySystem();
  printHash();

  char *test1 = dmalloc(4096 * 2);
  cprintf("test1 - %x\n", test1);
  printBuddySystem();
  printHash();

  char *test2 = dmalloc(4096);
  cprintf("test2 - %x\n", test2);
  printBuddySystem();
  printHash();

  char *test3 = dmalloc(4096 * 16);
  cprintf("test3 - %x\n", test3);
  printBuddySystem();
  printHash();

  char *test4 = dmalloc(4096);
  cprintf("test4 - %x\n", test4);
  printBuddySystem();
  printHash();

  char *test5 = dmalloc(4096 * 3);
  cprintf("test5 - %x\n", test5);
  printBuddySystem();
  printHash();

  char *test6 = dmalloc(4097);
  cprintf("test6 - %x\n", test6);
  printBuddySystem();
  printHash();

  cprintf("-----------------------------------\n\n");
  cprintf("free test start\n\n");

  cprintf("\ntest2 free\n");
  dfree(test2);
  printBuddySystem();
  printHash();

  cprintf("\ntest4 free\n");
  dfree(test4);
  printBuddySystem();
  printHash();

  cprintf("\ntest6 free\n");
  dfree(test6);
  printBuddySystem();
  printHash();

  cprintf("\ntest3 free\n");
  dfree(test3);
  printBuddySystem();
  printHash();

  cprintf("\ntest5 free\n");
  dfree(test5);
  printBuddySystem();
  printHash();

  cprintf("\ntest1 free\n");
  dfree(test1);
  printBuddySystem();
  printHash();
