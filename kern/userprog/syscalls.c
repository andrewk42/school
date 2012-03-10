#include <types.h>
#include <lib.h>
#include <proc.h>

/*
 * Definitions for IN-KERNEL entry points for system call implementations.
 */

pid_t
sys_fork(void) {
    kprintf("In sys_fork\n");


    return -1;
}

