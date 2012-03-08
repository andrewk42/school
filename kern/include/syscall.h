#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "opt-A2.h"

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

#if OPT_A2
pid_t sys_fork(void);
#endif // OPT_A2
int sys_reboot(int code);


#endif /* _SYSCALL_H_ */
