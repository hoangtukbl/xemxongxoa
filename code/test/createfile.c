#include "syscall.h"
#define maxlen 32
int
main()
{
    int len;
    char filename[maxlen + 1];
    /*Create a file*/
    if (Create("text.txt") == -1)
    {
    // xuất thông báo lỗi tạo tập tin
      PrintString("Tao file that bai\n");
    }
    else
    {
    // xuất thông báo tạo tập tin thành công
       //printf("Create file successfully");
       PrintString("Tao file thanh cong\n");
    }
    Halt();
} 
