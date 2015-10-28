// stats.h 
//	Routines for managing statistics about Nachos performance.
//
// DO NOT CHANGE -- these stats are maintained by the machine emulation.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "stats.h"
#include "../threads/system.h"

//----------------------------------------------------------------------
// Statistics::Statistics
// 	Initialize performance metrics to zero, at system startup.
//----------------------------------------------------------------------

Statistics::Statistics()
{
    totalTicks = idleTicks = systemTicks = userTicks = 0;
    numDiskReads = numDiskWrites = 0;
    numConsoleCharsRead = numConsoleCharsWritten = 0;
    numPageFaults = numPacketsSent = numPacketsRecvd = 0;
    numBursts = minBurst = maxBurst = totalBurst = 0;
    minCompletion = maxCompletion = totalCompletion = 0;
    squareCompletion = 0;
    totalWait = totalBlock = threadCount = errorEstimate = 0;
}

//----------------------------------------------------------------------
// Statistics::Print
// 	Print performance metrics, when we've finished everything
//	at system shutdown.
//----------------------------------------------------------------------

void
Statistics::Print()
{   
    printf("Ticks: total %d, idle %d, system %d, user %d\n", totalTicks, 
	idleTicks, systemTicks, userTicks);
    printf("Disk I/O: reads %d, writes %d\n", numDiskReads, numDiskWrites);
    printf("Console I/O: reads %d, writes %d\n", numConsoleCharsRead, 
	numConsoleCharsWritten);
    printf("Paging: faults %d\n", numPageFaults);
    printf("Network I/O: packets received %d, sent %d\n\n", numPacketsRecvd, 
	numPacketsSent);
    printf("[%s]\n", currentThread->getName());
    printf("Scheduling Policy: %d\n", scheduler->GetPolicy());
    double util = (totalBurst)/(double)totalTicks;
    printf("Number Of Threads: %d\n", threadCount);
    printf("CPU Busy Time: %d\n", totalTicks-idleTicks);
    printf("Execution Time: %d\n", totalTicks);
    printf("CPU Utilization: %lf\n",util);
    if(numBursts>0){
        int avg_burst = totalBurst/numBursts;
        printf("CPU Burst: maximum %d, minimum %d, average %d\n", maxBurst, minBurst, avg_burst);
    }
    if(threadCount>0){
        int avg_completion = totalCompletion/threadCount;
        long long int var_completion = squareCompletion/threadCount - (long long int)avg_completion*avg_completion;
        var_completion*=threadCount*threadCount;
        int avg_wait = totalWait/threadCount;
        //int avg_block = totalBlock/threadCount;
        printf("Average Wait Time: %d\n", avg_wait);
        //printf("Average Block Time: %d\n", avg_block);
        printf("Thread Completion: maximum %d, minimum %d, average %d, variance %lld\n",maxCompletion, minCompletion, totalCompletion, var_completion);
        
    }
    if(scheduler->GetPolicy()==2){
        printf("Estimation Error: %lf\n", (double)errorEstimate/totalBurst);
    }
}
