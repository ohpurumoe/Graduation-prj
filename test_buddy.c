#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

/*
nvme 기본 테스트
파일 시스템 올린 상태로 read, write 진행
*/
int
main(int argc, char* argv[])
{
  buddy_testing();
  exit();
}
