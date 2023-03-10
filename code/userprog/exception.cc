// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
//#include "machine.h"
#include "synchconsole.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions
//	is in machine.h.
//----------------------------------------------------------------------

#define MaxFileLength 32

void IncreasePC()
{
	// set previous program counter to current programcounter
	kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

	/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
	kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

	/* set next programm counter for brach execution */
	kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
}

void ExceptionHandler(ExceptionType which)
{
	int type = kernel->machine->ReadRegister(2);

	DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");

	switch (which)
	{
	case SyscallException:
		switch (type)
		{
		case SC_Halt:{
			DEBUG(dbgSys, "Shutdown, initiated by user program.\n");

			SysHalt();

			ASSERTNOTREACHED();
			break;
		}

		case SC_Create:
		{
			int virtAddr;
			char *filename;
			DEBUG('a', "\n SC_Create call ...");
			DEBUG('a', "\n Reading virtual address of filename");
			// Lấy tham số tên tập tin từ thanh ghi r4
			virtAddr = kernel->machine->ReadRegister(4);
			DEBUG('a', "\n Reading filename.");
			// MaxFileLength là = 32
			filename = User2System(virtAddr, MaxFileLength + 1);
			if (filename == NULL)
			{
				printf("\n Not enough memory in system");
				DEBUG('a', "\n Not enough memory in system");
				kernel->machine->WriteRegister(2, -1); // trả về lỗi cho chương
				// trình người dùng
				delete filename;
				IncreasePC();
				return;
			}
			DEBUG('a', "\n Finish reading filename.");
			// DEBUG('a',"\n File name : '"<<filename<<"'");
			//  Create file with size = 0
			//  Dùng đối tượng fileSystem của lớp OpenFile để tạo file,
			//  việc tạo file này là sử dụng các thủ tục tạo file của hệ điều
			//  hành Linux, chúng ta không quản ly trực tiếp các block trên
			//  đĩa cứng cấp phát cho file, việc quản ly các block của file
			//  trên ổ đĩa là một đồ án khác
			if (!kernel->fileSystem->Create(filename))
			{
				printf("\n Error create file '%s'", filename);
				kernel->machine->WriteRegister(2, -1);
				delete filename;
				IncreasePC();
				return;
			}
			kernel->machine->WriteRegister(2, 0); // trả về cho chương trình
			// người dùng thành công
			delete filename;
			IncreasePC();
			return;
			break;
		}
		case SC_Add:{
			DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");

			/* Process SysAdd Systemcall*/
			int result;
			result = SysAdd(/* int op1 */ (int)kernel->machine->ReadRegister(4),
							/* int op2 */ (int)kernel->machine->ReadRegister(5));

			DEBUG(dbgSys, "Add returning with " << result << "\n");
			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)result);

			/* Modify return point */
			IncreasePC();
			return;

			ASSERTNOTREACHED();

			break;}

		case SC_Open:
		{
			int bufAddr = kernel->machine->ReadRegister(4);
			int ftype = kernel->machine->ReadRegister(5);
			char *buf = User2System(bufAddr, MaxFileLength + 1);
			;
			// buf là filename

			int OpenFileID = -1;
			for (int i = 2; i < 20; i++)
			{
				if (kernel->fileSystem->openf[i] == NULL)
				{
					OpenFileID = i;
					break;
				}
			}
			if (OpenFileID != -1)
			{
				// chỉ xử lý khi type = 0 or = 1
				if (ftype == 0 or ftype == 1)
				{

					// printf("%d \n", OpenFileID);
					if (OpenFileID == -1)
					{
						printf("\n Cannot open file ");
						DEBUG(dbgSys, "\n Can not open file ...");
						kernel->machine->WriteRegister(2, -1);
					}
					else
					{
						kernel->fileSystem->openf[OpenFileID] = kernel->fileSystem->Open(buf, type);
						DEBUG(dbgSys, "\n Open file Success ...");
						// printf("\n Successfully open file ");
						kernel->machine->WriteRegister(2, OpenFileID);
					}
				}
				else if (ftype == 2) // stdin
				{
					kernel->machine->WriteRegister(2, 0);
				}
				else // stdout
				{
					kernel->machine->WriteRegister(2, 1);
				}
				delete[] buf;
				IncreasePC();
				return;
				break;
			}
			kernel->machine->WriteRegister(2, -1);
			delete[] buf;
			IncreasePC();
			return;
			break;
		}

		case SC_Close:
		{
			int fID = kernel->machine->ReadRegister(4); // Lay id cua file tu thanh ghi so 4
			int index = kernel->fileSystem->index;

			// opened [i] files, and want to close file No.[no] (no > i) --> go wrong
			if (fID > index)
			{
				printf("Close file failed \n");
				kernel->machine->WriteRegister(2, -1);
				IncreasePC();
				return;
				break;
			}
			if (fID >= 0 && fID <= kernel->fileSystem->index) // Chi xu li khi fid nam trong [0, 20]
			// va file phai co id nam trong cac id dang dc mo
			{
				if (kernel->fileSystem->openf[fID]) // neu mo file thanh cong
				{
					delete kernel->fileSystem->openf[fID]; // Xoa vung nho luu tru file
					kernel->fileSystem->openf[fID] = NULL; // Gan vung nho NULL
					kernel->machine->WriteRegister(2, 0);
					//printf("\n Successfully close file ");
					IncreasePC();
					return;
					break;
				}
			}
			printf("\n Cannot close file ");
			kernel->machine->WriteRegister(2, -1);
			IncreasePC();
			return;
			break;
		}

		case SC_Read:
		{
			int virtAddr = kernel->machine->ReadRegister(4);  // Lay dia chi cua tham so buffer tu thanh ghi so 4
			int charcount = kernel->machine->ReadRegister(5); // Lay charcount tu thanh ghi so 5
			int id = kernel->machine->ReadRegister(6);        // Lay id cua file tu thanh ghi so 6
			int OldPos;
			int NewPos;
			char *buf;
			int index = kernel->fileSystem->index;

			if (id > index || id < 0 || id == 0)
			{
				printf("\nKhong the read vi id nam ngoai bang mo ta file.");
				kernel->machine->WriteRegister(2, -1);
				IncreasePC();
				return;
			}

			if (kernel->fileSystem->openf[id] == NULL){
				printf("\nKhong the read vao mot file khong ton tai.");
				kernel->machine->WriteRegister(2, -1);
				IncreasePC();
				return;
			}

			if (kernel->fileSystem->openf[id]->type == 3) // Xet truong hop doc file stdout (type quy uoc la 3) thi tra ve -1
			{
				printf("\nKhong the read file stdout.");
				kernel->machine->WriteRegister(2, -1);
				IncreasePC();
				return;
			}
			OldPos = kernel->fileSystem->openf[id]->GetCurrentPos(); // Kiem tra thanh cong thi lay vi tri OldPos
			buf = User2System(virtAddr, charcount);                  // Copy chuoi tu vung nho User Space sang System Space voi bo dem buffer dai charcount
			// Xet truong hop doc file stdin (type quy uoc la 2)
			if (kernel->fileSystem->openf[id]->type == 2)
			{
				// Su dung ham Read cua lop SynchConsole de tra ve so byte thuc su doc duoc
				int size = 0;
				for (int i = 0; i < charcount; ++i)
				{
				size = size + 1;
				buf[i] = kernel->synchConsoleIn->GetChar();
				//Quy uoc chuoi ket thuc la \n
				if (buf[i] == '\n')
				{
					buf[i + 1] = '\0';
					break;
				}
				}
				buf[size] = '\0';
				System2User(virtAddr, size, buf);        // Copy chuoi tu vung nho System Space sang User Space voi bo dem buffer co do dai la so byte thuc su
				kernel->machine->WriteRegister(2, size); // Tra ve so byte thuc su doc duoc
				delete buf;
				IncreasePC();
				return;
			}
			// Xet truong hop doc file binh thuong thi tra ve so byte thuc su
			if ((kernel->fileSystem->openf[id]->Read(buf, charcount)) > 0)
			{
				// So byte thuc su = NewPos - OldPos
				NewPos = kernel->fileSystem->openf[id]->GetCurrentPos();
				// Copy chuoi tu vung nho System Space sang User Space voi bo dem buffer co do dai la so byte thuc su
				System2User(virtAddr, NewPos - OldPos, buf);
				kernel->machine->WriteRegister(2, NewPos - OldPos);
			}
			else
			{
				// Truong hop con lai la doc file co noi dung la NULL tra ve -2
				//printf("\nDoc file rong.");
				kernel->machine->WriteRegister(2, -2);
			}
			delete buf;
			IncreasePC();
			return;
		}

		case SC_Write:
		{
			int virtAddr = kernel->machine->ReadRegister(4);
			int charcount = kernel->machine->ReadRegister(5);
			int openf_id = kernel->machine->ReadRegister(6);
			int index = kernel->fileSystem->index;

			if (openf_id > index || openf_id < 0 || openf_id == 0)
			{
				printf("\nKhong the write vi id nam ngoai bang mo ta file.");
				kernel->machine->WriteRegister(2, -1);
				IncreasePC();
				return;
			}

			if (kernel->fileSystem->openf[openf_id] == NULL)
			{
				printf("\nKhong the write vi file nay khong ton tai.");
				kernel->machine->WriteRegister(2, -1);
				IncreasePC();
				return;
			}

			// read-only file
			if (kernel->fileSystem->openf[openf_id]->type == 1)
			{
				printf("\nKhong the write file stdin hoac file only read.");
				kernel->machine->WriteRegister(2, -1);
				IncreasePC();
				return;
			}

			// write to console
			char *buf = User2System(virtAddr, charcount);
			if (openf_id == 1)
			{
				int i = 0;
				while (buf[i] != '\0' && buf[i] != '\n')
				{
					kernel->synchConsoleOut->PutChar(buf[i]);
					i++;
				}
				buf[i] = '\n';
				kernel->synchConsoleOut->PutChar(buf[i]); // write last character

				kernel->machine->WriteRegister(2, i - 1);
				delete[] buf;
				IncreasePC();
				return;
			}

			// write into file
			int before = kernel->fileSystem->openf[openf_id]->GetCurrentPos();
			if ((kernel->fileSystem->openf[openf_id]->Write(buf, charcount)) != 0)
			{
				int after = kernel->fileSystem->openf[openf_id]->GetCurrentPos();
				System2User(virtAddr, after - before, buf);
				kernel->machine->WriteRegister(2, after - before + 1);
				delete[] buf;
				IncreasePC();
				return;
			}

			IncreasePC();
			return;
		}

		case SC_ReadChar:
		{
			char buffer;

			DEBUG(dbgSys, "\n SC_ReadChar call ...");

			// read char from keyboard
			buffer = kernel->synchConsoleIn->GetChar();

			// write value to register 2
			kernel->machine->WriteRegister(2, buffer);

			IncreasePC();
			return;
			ASSERTNOTREACHED();
			break;
		}

		case SC_PrintString:
		{
			DEBUG(dbgSys, "\n SC_PrintString call ...");
			int virtAddr;
			char *buffer;
			virtAddr = kernel->machine->ReadRegister(4); // Lay dia chi cua tham so buffer tu thanh ghi so 4
			buffer = User2System(virtAddr, 255);		 // Copy chuoi tu vung nho User Space sang System Space voi bo dem buffer dai 255 ki tu
			int length = 0;
			while (buffer[length] != 0)
				length++; // Dem do dai that cua chuoi
			for (int i = 0; i < length; i++)
			{
				kernel->synchConsoleOut->PutChar(buffer[i]);
			}
			delete buffer;
			DEBUG(dbgSys, "\n SC_PrintString call ...");
			IncreasePC();
			return;
		}

		case SC_PrintChar:
		{
			int ch;
			ch = kernel->machine->ReadRegister(4);
			kernel->synchConsoleOut->PutChar((char)ch);
			IncreasePC();
			return;
		}
		case SC_ReadInt:
		case SC_ReadNum:
		{
			int result = SysReadNum(); // <-- ksyscall.h
			kernel->machine->WriteRegister(2, result);
			IncreasePC();
			return;
		}

		case SC_PrintInt:
		case SC_PrintNum:
		{
			// Input: mot so integer
			// Output: khong co
			// Chuc nang: In so nguyen len man hinh console
			int number = kernel->machine->ReadRegister(4);
			if (number == 0)
			{
				kernel->synchConsoleOut->PutChar('0'); // In ra man hinh so 0
				IncreasePC();
				return;
			}

			/*Qua trinh chuyen so thanh chuoi de in ra man hinh*/
			bool isNegative = false; // gia su la so duong
			int numberOfNum = 0;	 // Bien de luu so chu so cua number
			int firstNumIndex = 0;

			if (number < 0)
			{
				isNegative = true;
				number = number * -1; // Nham chuyen so am thanh so duong de tinh so chu so
				firstNumIndex = 1;
			}

			int t_number = number; // bien tam cho number
			while (t_number)
			{
				numberOfNum++;
				t_number /= 10;
			}

			// Tao buffer chuoi de in ra man hinh
			char *buffer;
			int MAX_BUFFER = 255;
			buffer = new char[MAX_BUFFER + 1];
			for (int i = firstNumIndex + numberOfNum - 1; i >= firstNumIndex; i--)
			{
				buffer[i] = (char)((number % 10) + 48);
				number /= 10;
			}
			if (isNegative)
			{
				buffer[0] = '-';
				buffer[numberOfNum + 1] = 0;
				for (int i = 0; i < numberOfNum + 1; i++)
				{
					kernel->synchConsoleOut->PutChar(buffer[i]);
				}
				delete buffer;
				IncreasePC();
				return;
			}
			buffer[numberOfNum] = 0;
			for (int i = 0; i < numberOfNum; i++)
			{
				kernel->synchConsoleOut->PutChar(buffer[i]);
			}
			delete buffer;
			IncreasePC();
			return;
		}
		case SC_ReadString:
		{
			DEBUG(dbgSys, "\n SC_ReadString call ...");
			int addr, lenght;
			char *buffer;

			// read parameter from register 4 and 5
			addr = kernel->machine->ReadRegister(4);
			lenght = kernel->machine->ReadRegister(5);

			char ch;

			// copy string from user space to kernel space
			buffer = User2System(addr, lenght);
			int i = 0;

			// read string from keyboard
			while (true)
			{
				ch = kernel->synchConsoleIn->GetChar();
				if (ch == '\n')
					break;
				buffer[i] = ch;
				i++;
			}
			buffer += '\0';

			// copy string from kernel space to user space
			System2User(addr, lenght, buffer);

			IncreasePC();
			return;
			break;
		}

		
		case SC_Seek:
		{	
			//input vi tri(int), id cua file (openfileid)
			// output: -1 : loi, vi tri thuc su:thanh cong
			//cong dung: di chuyen con tro den vi tri can thiet trong file
			pos = kernel->machine->ReadRegister(4);
			int id = kernel->machine->ReadRegister(5);
			
			//kiem tra id
			if (id < 0 || id > 14){
				//printf("\n Khong the seek vi id nam ngoai bang mo ta file"
				kernel->machine->WriteRegister(2,-1);
				IncreasePC();
				return;
			}
			//kiem tra ton tai file
			if(kernel->fileSystem->openf[id]==NULL){
				//printf("\n khong the seek vi file khong ton tai");
				kernel->machine->WriteRegister(2,-1);
				IncreasePC();
				return;	
			}
			//kiem tra co goi seek tren console khong
			if (id == 0 || id == 1){
				//printf("\nkhong the seek tren file console");
				kernel->machine->WriteRegister(2,-1);
				IncreasePC();
				return;
			}
			pos = (pos == -1 ) ? kernel->fileSystem->openf[id]->Length(): pos;
			if (pos > kernel->fileSystem->openf[id]->Length() || pos < 0) {
				//printf("\n Khong the seek file den vi tri nay");
				kernel->machine->WriteRegister(2,-1);
			}
			else{
				kernel->fileSystem->openf[id]->Seek(pos);
				kernel->machine->WriteRegister(2,pos);
			}
			IncreasePC();
			return; 

		}	
		// case SC_Seek:
		// {
		// 	int pos = kernel->machine->ReadRegister(4);
		// 	int openf_id = kernel->machine->ReadRegister(5);

		// 	// seek into files: stdin, stdout, `out of domain` fileSystem
		// 	if (openf_id < 1 || openf_id > kernel->fileSystem->index || kernel->fileSystem->openf[openf_id] == NULL)
		// 	{
		// 		kernel->machine->WriteRegister(2, -1);
		// 		break;
		// 	}

		// 	int len = kernel->fileSystem->openf[openf_id]->Length();	// file size

		// 	if (pos > len)	// try to move file ptr to pos, pos > len --> wrong
		// 	{
		// 		kernel->machine->WriteRegister(2, -1);
		// 		break;
		// 	}

		// 	if (pos == -1)	// move file ptr to the begining of file
		// 		pos = len;

		// 	kernel->fileSystem->openf[openf_id]->Seek(pos);
		// 	kernel->machine->WriteRegister(2, pos);
		// 	break;
		// }
		default:{
			cerr << "Unexpected system call " << type << "\n";
			break;
		}
		break;
		}
	default:
		cerr << "Unexpected user mode exception" << (int)which << "\n";
		break;
	}
	
	ASSERTNOTREACHED();
}
