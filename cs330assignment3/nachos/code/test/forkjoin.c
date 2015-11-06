#include "syscall.h"
#include "synchop.h"
int
main()
{
    int x;
    int *array = (int*)system_ShmAllocate(3*sizeof(int));
    array[0] = 1;
    array[1] = 2;
    array[2] = 3;
    int sleep_start, sleep_end;
    int id = system_SemGet(2);
    system_PrintInt(id);
    id = system_SemGet(3);
    system_PrintInt(id);
    system_PrintString("Parent PID: ");
    system_PrintInt(system_GetPID());
    system_PrintChar('\n');
        system_PrintInt(array[0]);
    system_PrintChar('\n');
        system_PrintInt(array[1]);
    system_PrintChar('\n');
        system_PrintInt(array[2]);
    system_PrintChar('\n');
    x = system_Fork();
    array[1] = 4;
    if (x == 0) {
        array[0] = 5;
    system_SemOp(id,-1);
        system_PrintInt(array[0]);
    system_PrintChar('\n');
        system_PrintInt(array[1]);
    system_PrintChar('\n');
        system_PrintInt(array[2]);
    system_PrintChar('\n');
       system_PrintString("Child PID: ");
       system_PrintInt(system_GetPID());
       system_PrintChar('\n');
       system_PrintString("Child's parent PID: ");
       system_PrintInt(system_GetPPID());
       system_PrintChar('\n');
       sleep_start = system_GetTime();
       system_Sleep(100);
       sleep_end = system_GetTime();
       system_PrintString("Child called sleep at time: ");
       system_PrintInt(sleep_start);
       system_PrintChar('\n');
       system_PrintString("Child returned from sleep at time: ");
       system_PrintInt(sleep_end);
       system_PrintChar('\n');
       system_PrintString("Child executed ");
       system_PrintInt(system_GetNumInstr());
       system_PrintString(" instructions.\n");
    system_SemOp(id,1);
    }
    else {
        //system_Sleep(1000000);
    system_SemOp(id,-1);
        system_PrintInt(array[0]);
    system_PrintChar('\n');
        system_PrintInt(array[1]);
    system_PrintChar('\n');
        system_PrintInt(array[2]);
    system_PrintChar('\n');
       system_PrintString("Parent after fork waiting for child: ");
       system_PrintInt(x);
       system_PrintChar('\n');
       system_Join(x);
       system_PrintString("Parent executed ");
       system_PrintInt(system_GetNumInstr());
       system_PrintString(" instructions.\n");
    system_SemOp(id,1);
    }
    return 0;
}
