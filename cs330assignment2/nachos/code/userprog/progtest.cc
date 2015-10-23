// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"

extern void ForkStartFunction (int dummy);
//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

void
StartProcess(char *filename)
{
    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;

    if (executable == NULL) {
	printf("Unable to open file %s\n", filename);
	return;
    }
    space = new AddrSpace(executable);    
    currentThread->space = space;

    delete executable;			// close file

    space->InitRegisters();		// set the initial register values
    space->RestoreState();		// load page table register

    machine->Run();			// jump to the user progam
    ASSERT(FALSE);			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}

// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

void BatchProcess(char* filename){
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    OpenFile *file = fileSystem->Open(filename);
    if (file == NULL) {
        printf("Unable to open file %s\n", filename);
        return;
    }

    int filelength = file->Length();
    char Buffer[filelength];
    OpenFile *executable;
    NachOSThread *thread;
    AddrSpace *space;
    
    file->ReadAt(Buffer, (filelength-1), 0);
    DEBUG('s', "running batch jobs from \"%s\"\n", filename);
  
    int i=0, j=0, k=0, policy=1;
    char name[100];
    char priority[10];
    
    while(i<filelength){
        if(Buffer[i]<0) break;
        if(Buffer[i]==' ' || Buffer[i]=='\n'){
            name[j]='\0';
            if(Buffer[i]==' ') i++;
            while(Buffer[i]!='\n' && i<filelength && Buffer[i]>=0){
                priority[k] = Buffer[i];
                i++;k++;
            }
            priority[k]='\0';
            i++;
            if(policy){
                scheduler->SetPolicy(atoi(name));
                policy=0;
            }
            else{
                executable = fileSystem->Open(name);
                if (executable == NULL) {
                    printf("Unable to open file %s\n", filename);
                    return;
                }
                space = new AddrSpace(executable);
                name[j]='|';name[j+1]='0'+thread_index%10;name[j+2]='\0';
                thread = new NachOSThread(strdup(name));
                thread->space = space;
                if(priority[0]!='\0') thread->SetPriority(atoi(priority));
                delete executable;			// close file

                space->InitRegisters();		// set the initial register values
                thread->SaveUserState();		// load page table register
                thread->ThreadFork(ForkStartFunction, 0);	// Make it ready for a later context switch
                
                DEBUG('s',"%s %s\n",name,priority);
            }
            j=0;k=0;
        }
        else{
            name[j] = Buffer[i];
            i++;j++;
        }
    }
    exitThreadArray[currentThread->GetPID()] = true;
    currentThread->PutThreadToSleep();
    currentThread->Exit(true,0);
}

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void 
ConsoleTest (char *in, char *out)
{
    char ch;

    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    
    for (;;) {
	readAvail->P();		// wait for character to arrive
	ch = console->GetChar();
	console->PutChar(ch);	// echo it!
	writeDone->P() ;        // wait for write to finish
	if (ch == 'q') return;  // if q, quit
    }
}
