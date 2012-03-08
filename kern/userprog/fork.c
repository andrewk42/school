/*
 * Implementation code for sys_fork()
 *
 */

#include <types.h>

pid_t
sys_fork(void) {
    kprintf("In sys_fork\n");
    return -1;
}
