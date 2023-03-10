#include "syscall.h"
#define maxlen 32
int main()
{
    // int len;
    // char filename[maxlen + 1];
    // char buf[32];
    // /*Create a file*/
    // int CloseStatus;
    // int byteRead; // so byte thuc su doc duoc
    // int OpenFileID = Open("text.txt", 0);
    // if (OpenFileID != -1)
    // {
    //     PrintString("Mo file thanh cong\n");
    //     Write("Hello", 5, OpenFileID); // Lỗi không read xong write ngay được, chỉ read hoặc write được 1 lần duy nhất trong 1 ctrinh
	// Seek(2,OpenFileID);
	// Write("hh",2,OpenFileID);
    //     //byteRead = Read(buf, 5, OpenFileID);
    //     //PrintInt(byteRead);
    //     //PrintString(buf);
    //     CloseStatus = Close(OpenFileID);
    //     if (CloseStatus == 0)
    //         PrintString("Dong file thanh cong\n");
    // }

    int RemoveFileID = Remove("text.txt");
    if (RemoveFileID != -1)
        PrintString("Da xoa file");
    else
        PrintString("chua xoa dc");
    Halt();
}
