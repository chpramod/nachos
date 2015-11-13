// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    unsigned int i, size;
    //unsigned vpn, offset;
    //TranslationEntry *entry;
    //unsigned int pageFrame;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages+numPagesAllocated <= NumPhysPages);		// check we're not trying
										// to run anything too big --
										// at least until we have
										// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
	pageTable[i].virtualPage = i;
	pageTable[i].physicalPage = -1;
	pageTable[i].valid = FALSE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;
        pageTable[i].shared = FALSE;// if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
    }
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
    //bzero(&machine->mainMemory[numPagesAllocated*PageSize], size);
 
    //numPagesAllocated += numPages;

// then, copy in the code and data segments into memory
    /*if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
        vpn = noffH.code.virtualAddr/PageSize;
        offset = noffH.code.virtualAddr%PageSize;
        entry = &pageTable[vpn];
        pageFrame = entry->physicalPage;
        executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
        vpn = noffH.initData.virtualAddr/PageSize;
        offset = noffH.initData.virtualAddr%PageSize;
        entry = &pageTable[vpn];
        pageFrame = entry->physicalPage;
        executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }*/

}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace (AddrSpace*) is called by a forked thread.
//      We need to duplicate the address space of the parent.
//----------------------------------------------------------------------

AddrSpace::AddrSpace(AddrSpace *parentSpace)
{
    numPages = parentSpace->GetNumPages();
    noffH = parentSpace->noffH;
    unsigned i,j,k,validPages, sharedPages,startAddrParent,startAddrChild;

    ASSERT(numPages+numPagesAllocated <= NumPhysPages);                // check we're not trying
                                                                                // to run anything too big --
                                                                                // at least until we have
                                                                                // virtual memory

    DEBUG('z', "Initializing address space, num pages %d, size %d\n",
                                        numPages, numPages*PageSize);
    // first, set up the translation
    TranslationEntry* parentPageTable = parentSpace->GetPageTable();
    pageTable = new TranslationEntry[numPages];
    for (i = 0,j=0, validPages=0, sharedPages = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i;
        if(parentPageTable[i].shared){
            pageTable[i].physicalPage = parentPageTable[i].physicalPage;
            sharedPages++;
        }
        else if(parentPageTable[i].valid){
            if(machine->free_index<=0){
                pageTable[i].physicalPage = j+numPagesAllocated;
                j++;
            }
            else {
                machine->free_index--;
                pageTable[i].physicalPage = machine->freePages[machine->free_index];
                }
            startAddrParent = parentPageTable[i].physicalPage*PageSize;
            startAddrChild = pageTable[i].physicalPage*PageSize;
            for(k=0;k<PageSize;k++){
                machine->mainMemory[startAddrChild+k] = machine->mainMemory[startAddrParent+k];
            }
            machine->pageArray[pageTable[i].physicalPage] = currentThread->GetPID();
            validPages++;
        }
        else pageTable[i].physicalPage = -1;
        pageTable[i].valid = parentPageTable[i].valid;
        pageTable[i].use = parentPageTable[i].use;
        pageTable[i].dirty = parentPageTable[i].dirty;
        pageTable[i].readOnly = parentPageTable[i].readOnly;
        pageTable[i].shared = parentPageTable[i].shared;  	// if the code segment was entirely on
                                        			// a separate page, we could set its
                                        			// pages to be read-only
    }
    
    stats->numPageFaults += validPages;
    //numPagesAllocated += numPages;
    numPagesAllocated += j;
}

int
AddrSpace::ShmAllocate(int size){
    
    int i,j, extraPages = (size+PageSize-1)/PageSize;
    
    ASSERT(extraPages+numPagesAllocated <= NumPhysPages);
    
    TranslationEntry* oldPageTable = pageTable;
    pageTable = new TranslationEntry[numPages + extraPages];
    for (i = 0; i < numPages; i++) {
        pageTable[i] = oldPageTable[i];
    }
    for (i = numPages, j=0; i < numPages + extraPages; i++){
        pageTable[i].virtualPage = i;
        
        if(machine->free_index<=0){
            pageTable[i].physicalPage = j+numPagesAllocated;
            j++;
        }
        else {
            machine->free_index--;
            pageTable[i].physicalPage = machine->freePages[machine->free_index];
        }
        machine->pageArray[pageTable[i].physicalPage] = currentThread->GetPID();
	pageTable[i].valid = TRUE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;
        pageTable[i].shared = TRUE;
        bzero(&machine->mainMemory[pageTable[i].physicalPage*PageSize], PageSize);
        DEBUG('z',"\t\t\t\t[SHM %d]\n",machine->pageArray[pageTable[i].physicalPage]);
    }
    
    
    
    numPages += extraPages;
    numPagesAllocated += j;
    stats->numPageFaults += extraPages;
    delete oldPageTable;
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
    return (unsigned)(numPages-extraPages)*PageSize;
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Deallocate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{   
    for (int i = 0; i < numPages; i++){
        if(pageTable[i].valid && !pageTable[i].shared){
            machine->pageArray[pageTable[i].physicalPage] = -1; // Page marked as free
            machine->freePages[machine->free_index] = pageTable[i].physicalPage;
            machine->free_index++;
            DEBUG('z',"\t\t\t[%d]\n",pageTable[i].physicalPage);
        }
    }
    
    delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

unsigned
AddrSpace::GetNumPages()
{
   return numPages;
}

TranslationEntry*
AddrSpace::GetPageTable()
{
   return pageTable;
}
