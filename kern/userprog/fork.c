/*
 * Implementation code for sys_fork()
 *
 */

#include <types.h>
#include <machine/trapframe.h>

pid_t
sys_fork(void) {
    kprintf("In sys_fork\n");

    // Allocate a pid for the new process.

    // Create new process' address space from current process

    // Create the new process' first thread of execution

    // Create copies of other resources (e.g. open files, sockets, etc.)

    // Have the new thread (implementing the new process) run forkentry()
    //md_forkentry(tf);

    return -1;
}
