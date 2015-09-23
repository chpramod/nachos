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
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "console.h"
#include "synch.h"
#include "addrspace.h"

#define MAX_FILENAME 100

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
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------
static Semaphore *readAvail;
static Semaphore *writeDone;

static void ReadAvail(int arg) {
    readAvail->V();
}

static void WriteDone(int arg) {
    writeDone->V();
}

static void ConvertIntToHex(unsigned v, Console *console) {
    unsigned x;
    if (v == 0) return;
    ConvertIntToHex(v / 16, console);
    x = v % 16;
    if (x < 10) {
        writeDone->P();
        console->PutChar('0' + x);
    } else {
        writeDone->P();
        console->PutChar('a' + x - 10);
    }
}

void
ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2);
    int memval, vaddr, printval, tempval, exp;
    unsigned printvalus; // Used for printing in hex
    if (!initializedConsoleSemaphores) {
        readAvail = new Semaphore("read avail", 0);
        writeDone = new Semaphore("write done", 1);
        initializedConsoleSemaphores = true;
    }
    Console *console = new Console(NULL, NULL, ReadAvail, WriteDone, 0);
    ;

    if ((which == SyscallException) && (type == syscall_Halt)) {
        DEBUG('a', "Shutdown, initiated by user program.\n");
        interrupt->Halt();
    } else if ((which == SyscallException) && (type == syscall_PrintInt)) {
        printval = machine->ReadRegister(4);
        if (printval == 0) {
            writeDone->P();
            console->PutChar('0');
        } else {
            if (printval < 0) {
                writeDone->P();
                console->PutChar('-');
                printval = -printval;
            }
            tempval = printval;
            exp = 1;
            while (tempval != 0) {
                tempval = tempval / 10;
                exp = exp * 10;
            }
            exp = exp / 10;
            while (exp > 0) {
                writeDone->P();
                console->PutChar('0' + (printval / exp));
                printval = printval % exp;
                exp = exp / 10;
            }
        }
        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
    } else if ((which == SyscallException) && (type == syscall_PrintChar)) {
        writeDone->P();
        console->PutChar(machine->ReadRegister(4)); // echo it!
        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
    } else if ((which == SyscallException) && (type == syscall_PrintString)) {
        vaddr = machine->ReadRegister(4);
        machine->ReadMem(vaddr, 1, &memval);
        while ((*(char*) &memval) != '\0') {
            writeDone->P();
            console->PutChar(*(char*) &memval);
            vaddr++;
            machine->ReadMem(vaddr, 1, &memval);
        }
        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
    } else if ((which == SyscallException) && (type == syscall_PrintIntHex)) {
        printvalus = (unsigned) machine->ReadRegister(4);
        writeDone->P();
        console->PutChar('0');
        writeDone->P();
        console->PutChar('x');
        if (printvalus == 0) {
            writeDone->P();
            console->PutChar('0');
        } else {
            ConvertIntToHex(printvalus, console);
        }
        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
    } else if ((which == SyscallException) && (type == syscall_GetPA)) {
        int phyaddr;
        ExceptionType ans = machine->Translate((unsigned) machine->ReadRegister(4), &phyaddr, 4, false);
        if (ans == AddressErrorException || ans == PageFaultException || ans == BusErrorException) machine->WriteRegister(2, -1);
        machine->WriteRegister(2, phyaddr);
        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
    } else if ((which == SyscallException) && (type == syscall_GetReg)) {
        printvalus = (unsigned) machine->ReadRegister((unsigned) machine->ReadRegister(4));
        machine->WriteRegister(2, printvalus);
        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
    } else if ((which == SyscallException) && (type == syscall_NumInstr)) {
        printvalus = stats->userTicks;
        machine->WriteRegister(2, printvalus);
        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
    } else if ((which == SyscallException) && (type == syscall_Time)) {
        printvalus = (unsigned) stats->totalTicks;
        machine->WriteRegister(2, printvalus);
        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
    } else if ((which == SyscallException) && (type == syscall_Yield)) {
        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
        currentThread->YieldCPU();
    } else if ((which == SyscallException) && (type == syscall_GetPID)) {
        machine->WriteRegister(2, currentThread->getPID());
        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
    } else if ((which == SyscallException) && (type == syscall_GetPPID)) {
        machine->WriteRegister(2, currentThread->getPPID());
        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
    } else if ((which == SyscallException) && (type == syscall_Sleep)) {
        int times = (unsigned) machine->ReadRegister(4);
        DEBUG('t', "Sleep Entry.\n");
        if (times == 0) {
            IntStatus oldLevel = interrupt->SetLevel(IntOff);
            currentThread->YieldCPU();
            (void) interrupt->SetLevel(oldLevel);
        } else {
            scheduler->Sleep(currentThread, times + stats->totalTicks);
            IntStatus oldLevel = interrupt->SetLevel(IntOff);
            currentThread->PutThreadToSleep();
            (void) interrupt->SetLevel(oldLevel);
        }
        DEBUG('t', "Sleep Exit.\n");
        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
    } else if ((which == SyscallException) && (type == syscall_Exec)) {
        char filename[MAX_FILENAME + 1];
        int i = 0;
        vaddr = machine->ReadRegister(4);
        machine->ReadMem(vaddr, 1, &memval);
        while ((*(char*) &memval) != '\0') {
            if (i == MAX_FILENAME) {
                printf("Filename exceeds %d characters", MAX_FILENAME);
                // Advance program counters.
                machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
                machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
                machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);

                return;
            }
            filename[i] = *(char*) &memval;
            i++;
            vaddr++;
            machine->ReadMem(vaddr, 1, &memval);
        }
        if (i == MAX_FILENAME + 1) {
            printf("Filename exceeds %d characters", MAX_FILENAME);
            // Advance program counters.
            machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
            machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
            machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);

            return;
        }
        filename[i] = '\0';
        OpenFile *executable = fileSystem->Open(filename);
        AddrSpace *space;

        if (executable == NULL) {
            printf("Unable to open file %s\n", filename);
            // Advance program counters.
            machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
            machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
            machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);

            return;
        }

        space = new AddrSpace(executable);
        currentThread->space = space;

        delete executable; // close file

        space->InitRegisters(); // set the initial register values
        space->RestoreState(); // load page table register

        machine->Run(); // jump to the user progam
        ASSERT(FALSE); // machine->Run never returns;
        // the address space exits
        // by doing the syscall "exit"

    } else if ((which == SyscallException) && (type == syscall_Fork)) {        
        // Advance program counters.
            machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
            machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
            machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
            
        NachOSThread* fork_child;
        fork_child = new NachOSThread("child");
        fork_child->parent = currentThread;
        fork_child->setPPID(currentThread->getPID());
        //currentThread->AppendChild(fork_child);
        AddrSpace* space;
        IntStatus oldLevel = interrupt->SetLevel(IntOff);
        space = new AddrSpace(NULL,true);
        (void) interrupt->SetLevel(oldLevel);
        fork_child->space = space;

        machine->WriteRegister(2, 0);
        fork_child->SaveUserState();
        machine->WriteRegister(2, fork_child->getPID());
        
        fork_child->ThreadFork(&fork_start,0);
        
    } 
    else if ((which == SyscallException) && (type == syscall_Join)) {
        int child_pid = machine->ReadRegister(4);
        
        // Advance program counters.
            machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
            machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
            machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
           
    }
    else if ((which == SyscallException) && (type == syscall_Exit)) {
        IntStatus oldLevel = interrupt->SetLevel(IntOff);
        extern process* processArray;
        int exitstatus = machine->ReadRegister(4);
        
        extern int totalThreads;
        if(totalThreads==0) interrupt->Halt();
        
        if(currentThread->parent!=NULL){
            if(processArray[currentThread->getPID()].parentWait==PARENT_WAITING){
                scheduler->ReadyToRun(currentThread->parent);
            }
        }
        processArray[currentThread->getPID()].aliveStatus=DEAD;
        processArray[currentThread->getPID()].exitStatus=exitstatus;
        (void) interrupt->SetLevel(oldLevel);
        currentThread->FinishThread();
        // Advance program counters.
            machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
            machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
            machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
           
    }   
    else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}
