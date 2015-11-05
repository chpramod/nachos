#include "syscall.h"

int
main()
{
    system_PrintString("Before calling Exec.\n");
    //system_Exec("../test/vectorsum");
    system_PrintString("Returned from Exec.\n"); // Should never return

    int *array = (int*)system_ShmAllocate(2*sizeof(int));
    array[0] = 1;
    array[1] = 2;
    system_PrintString("Array[0]: ");
    system_PrintInt(array[0]);
    system_PrintChar('\n');
    system_PrintString("Array[1]: ");
    system_PrintInt(array[1]);
    system_PrintChar('\n');
    return 0;
}
