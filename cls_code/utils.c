#include "lib.h"
/*
int atoi(char* str, byte strSize)
{
  int result=0;
  byte i=0;
  byte negativeBool=0;
  if(str[0]=='-')
  {
    negativeBool=1;
    i++;
  }

  result=str[i++]-0x30;
  while(i<strSize)
  {
    result=result*10;
    result = result + (str[i++]-0x30);
  }

  if(negativeBool) result=result*-1;
  return result;
}
*/