#include"server.h"
#include"server.c"

int main(){
  printf("Test cases for server\n");
  printf("server.c tests\n");
  printf("====================================\n");
  printf("tests for chunk matching");
  int matchCount = 0;
  if(match_chunk("test","test", &matchCount) && matchCount == 4){
    printf("Matching \"test\" with \"test\" passed\n");
  } else {
    printf("Matching \"test\" with \"test\" failed.\n");
  }  
  printf("end server.c tests\n");

  return 0;
}