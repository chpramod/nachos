// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "scheduler.h"
#include "system.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads to empty.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{ 
    readyList = new List;
    this->SetPolicy(-1);
    alpha = 0.5;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete readyList; 
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (NachOSThread *thread)
{
    //printf("[Ready %d %d]\n",thread->GetPID(), stats->totalTicks);
    DEBUG('t', "Putting thread %s on ready list.\n", thread->getName());
    //printf("[pid %d] appended\n",thread->GetPID());
    thread->setStatus(READY);
    if(policy==2){
        if(thread->previous_burst > 0){
            float estimated_time = thread->estimated_burst + alpha*(thread->previous_burst - thread->estimated_burst);
            if(thread->estimated_burst > thread->previous_burst) stats->errorEstimate+= thread->estimated_burst - thread->previous_burst;
            else stats->errorEstimate+= thread->previous_burst - thread->estimated_burst;
            //printf("[%d %d]\n",thread->GetPID(), stats->errorEstimate);
            thread->estimated_burst = estimated_time;
        }
        DEBUG('j',"[PID %d Estimated_Time %f]\n",thread->GetPID(),thread->estimated_burst);
        readyList->SortedInsert((void*)thread,(int)thread->estimated_burst);
    }
    else readyList->Append((void *)thread);
    thread->block_time = stats->totalTicks - thread->last_block;
    thread->last_wait = stats->totalTicks;
    //Print();printf("\n");
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

NachOSThread *
Scheduler::FindNextToRun ()
{   
    if(policy==2){
        NachOSThread* thread = (NachOSThread*)readyList->SortedRemove(NULL);;
        if(thread!=NULL) DEBUG('j',"[PID %d Estimated_Time %d]\n",thread->GetPID(),thread->estimated_burst);
        return thread;
    }
    else if(policy>=7 && policy<=10){
        ListElement* min_list_element = readyList->first;
        ListElement* list_element = min_list_element;
        NachOSThread* thread;
        if(min_list_element == NULL) {
            thread = NULL;
        }
        else{
            int min = ((NachOSThread* )min_list_element->item)->GetPriority();
            NachOSThread* temp = (NachOSThread* )min_list_element->item;
            thread = temp;

            while(list_element!=NULL) {
               temp = (NachOSThread *)list_element->item;

               if(temp->GetPriority() <= min) {
                   min_list_element = list_element;
                   min = temp->GetPriority();
                   thread = temp;
               }
               list_element=list_element->next;
            }

            list_element = readyList->first;
            if (list_element == min_list_element) {
                thread = (NachOSThread* )readyList->Remove();
            }
            else{
                ListElement* list_temp = list_element;
                list_element = list_element->next;
                while(list_element!=NULL) {
                    if(list_element == min_list_element) {
                        list_temp->next = list_element->next;
                            if (list_element == readyList->last) {
                            readyList->last = list_temp;
                        }
                    }
                    list_temp = list_element;
                    list_element = list_element->next;
                }
                delete min_list_element;
            }
            currentThread->cpu_count+=currentThread->previous_burst;
            for(int i=0;i<thread_index;i++) if(exitThreadArray[i]==FALSE) threadArray[i]->cpu_count/=2;
        }
        return thread;

    }
    else return (NachOSThread* )readyList->Remove();
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//----------------------------------------------------------------------

void
Scheduler::Run (NachOSThread *nextThread, bool exit)
{
    /*printf("[Run %d %d]\n",nextThread->GetPID(), stats->totalTicks);
    if(stats->totalTicks >= 103645){
        for(int i=0;i<10;i++){
            if(exitThreadArray[i+1]==false) printf("[%s %d]\n",threadArray[i+1]->getName(),threadArray[1+i]->block_time);
        }
    }*/
    NachOSThread *oldThread = currentThread;
    //printf("[pid %d] next [pid %d] old\n",nextThread->GetPID(), currentThread->GetPID());
#ifdef USER_PROGRAM			// ignore until running user programs 
    if (currentThread->space != NULL) {	// if this thread is a user program,
        currentThread->SaveUserState(); // save the user's CPU registers
	currentThread->space->SaveState();
    }
#endif
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    currentThread = nextThread;		    // switch to the next thread
    currentThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG('t', "Switching from thread \"%s\" to thread \"%s\"\n",
	  oldThread->getName(), nextThread->getName());
    //if(currentThread->GetPID()==1) printf("\t\t\t[WAIT %d %d",stats->totalTicks - currentThread->last_wait,currentThread->wait_time);
    currentThread->wait_time += stats->totalTicks - currentThread->last_wait;
    //printf(" %d]\n", currentThread->wait_time);
    currentThread->last_burst = stats->totalTicks;
    currentThread->last_round_robin = 0;
    
    if(exit){
        float estimated_time = oldThread->estimated_burst + alpha*(oldThread->previous_burst - oldThread->estimated_burst);
        if(oldThread->estimated_burst > oldThread->previous_burst) stats->errorEstimate+= oldThread->estimated_burst - oldThread->previous_burst;
        else stats->errorEstimate+= oldThread->previous_burst - oldThread->estimated_burst;
        
        stats->totalBurst += oldThread->cpu_burst;
        //printf("[Bursts %d %d %d]\n", oldThread->cpu_burst, stats->totalBurst, stats->totalTicks);
        stats->numBursts += oldThread->burst_count - oldThread->zero_burst;
        if(stats->maxCompletion < oldThread->end_time) stats->maxCompletion = oldThread->end_time;
        if(stats->minCompletion > oldThread->end_time || stats->minCompletion == 0) stats->minCompletion = oldThread->end_time;
        //printf("[stats->totalCompletion %d %d %d]\n",oldThread->GetPID(), oldThread->end_time, stats->totalCompletion);
        stats->totalCompletion += oldThread->end_time/stats->threadCount;
        //printf("[stats->totalCompletion %d %d %d]\n",oldThread->GetPID(), oldThread->end_time, stats->totalCompletion);
        long long int square_end = (oldThread->end_time/stats->threadCount)*(oldThread->end_time/stats->threadCount);
        //printf("[stats->squareCompletion %d %d %lld %lld]\n",oldThread->GetPID(),oldThread->end_time, square_end, stats->squareCompletion);
        stats->squareCompletion += square_end;
        //printf("[stats->squareCompletion %d %d %lld %lld]\n",oldThread->GetPID(),oldThread->end_time, square_end, stats->squareCompletion);
        stats->totalWait += oldThread->wait_time;
        stats->totalBlock += oldThread->block_time;
    }
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".
    _SWITCH(oldThread, nextThread);
    DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());

    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in NachOSThread::FinishThread()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
	threadToBeDestroyed = NULL;
    }
    
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {		// if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
	currentThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::Tail
//      This is the portion of Scheduler::Run after _SWITCH(). This needs
//      to be executed in the startup function used in fork().
//----------------------------------------------------------------------

void
Scheduler::Tail ()
{
    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in NachOSThread::FinishThread()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }

#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {         // if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
        currentThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    printf("Ready list contents:\n");
    readyList->Mapcar((VoidFunctionPtr) ThreadPrint);
}

void
Scheduler::SetPolicy(int policy_value){
    policy = policy_value;
    switch(policy){
        case 1: // Non-preemptive default NachOS scheduling
            quanta = 120;
            break;
        case 2: // Non-preemptive shortest next CPU burst first algorithm
            quanta = 120;
            break;
        case 3: // Round-robin
            quanta = 90;
            break;
        case 4: // Round-robin 
            quanta = 60;
            break;
        case 5: // Round-robin 
            quanta = 30;
            break;
        case 6: // Round-robin 
            quanta = 20;
            break;
        case 7: // UNIX Scheduler 
            quanta = 90;
            break;
        case 8: // UNIX Scheduler 
            quanta = 60;
            break;
        case 9: // UNIX Scheduler 
            quanta = 30;
            break;
        case 10: // UNIX Scheduler 
            quanta = 20;
            break;
        default:
            quanta = 98;
            break;
    }
}

int
Scheduler::GetPolicy(){
    return policy;
}

int
Scheduler::GetQuanta(){
    return quanta;
}
