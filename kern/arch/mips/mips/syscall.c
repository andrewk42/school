#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <kern/callno.h>
#include <syscall.h>

#include "opt-A2.h"


/*
 * System call handler.
 *
 * A pointer to the trapframe created during exception entry (in
 * exception.S) is passed in.
 *
 * The calling conventions for syscalls are as follows: Like ordinary
 * function calls, the first 4 32-bit arguments are passed in the 4
 * argument registers a0-a3. In addition, the system call number is
 * passed in the v0 register.
 *
 * On successful return, the return value is passed back in the v0
 * register, like an ordinary function call, and the a3 register is
 * also set to 0 to indicate success.
 *
 * On an error return, the error code is passed back in the v0
 * register, and the a3 register is set to 1 to indicate failure.
 * (Userlevel code takes care of storing the error code in errno and
 * returning the value -1 from the actual userlevel syscall function.
 * See src/lib/libc/syscalls.S and related files.)
 *
 * Upon syscall return the program counter stored in the trapframe
 * must be incremented by one instruction; otherwise the exception
 * return code will restart the "syscall" instruction and the system
 * call will repeat forever.
 *
 * Since none of the OS/161 system calls have more than 4 arguments,
 * there should be no need to fetch additional arguments from the
 * user-level stack.
 *
 * Watch out: if you make system calls that have 64-bit quantities as
 * arguments, they will get passed in pairs of registers, and not
 * necessarily in the way you expect. We recommend you don't do it.
 * (In fact, we recommend you don't use 64-bit quantities at all. See
 * arch/mips/include/types.h.)
 */

void
mips_syscall(struct trapframe *tf)
{
	int callno;
	int32_t retval;
	int err;

	assert(curspl==0);

	callno = tf->tf_v0;

	/*
	 * Initialize retval to 0. Many of the system calls don't
	 * really return a value, just 0 for success and -1 on
	 * error. Since retval is the value returned on success,
	 * initialize it to 0 by default; thus it's not necessary to
	 * deal with it except for calls that return other values, 
	 * like write.
	 */

	retval = 0;

	switch (callno) {
	    case SYS_reboot:
		    err = sys_reboot(tf->tf_a0);
		    break;

#if OPT_A2
        case SYS__exit:
            kprintf("Call to _exit()\n");
            err = ENOSYS;
            break;

        case SYS_execv:
            kprintf("Call to execv()\n");
            err = ENOSYS;
            break;

        case SYS_fork:
            retval = sys_fork();

            // Map sys_fork error to a proper errno
            switch (retval) {
                case -1:
                    err = EAGAIN;
                    break;
                case -2:
                    err = ENOMEM;
                    break;
                /*
                 * If sys_fork() returned a value other than -1 or -2,
                 * then it should have been sucessful and returned
                 * either 0 or a proper pid.
                 */
                default:
                    err = 0;
                    break;
            }

            break;

        case SYS_waitpid:
            kprintf("Call to waitpid()\n");
            err = ENOSYS;
            break;

        case SYS_open:
            kprintf("Call to open()\n");
            err = ENOSYS;
            break;

        case SYS_read:
            kprintf("Call to read()\n");
            err = ENOSYS;
            break;

        case SYS_write:
            //kprintf("Call to write()\n");
            kprintf((const char *)(tf->tf_a1));
            break;

        case SYS_close:
            kprintf("Call to close()\n");
            err = ENOSYS;
            break;

        case SYS_getpid:
            kprintf("Call to getpid()\n");
            err = ENOSYS;
            break;
#else // OPT_A2
	    /* Add stuff here */
#endif
 
	    default:
		    kprintf("Unknown syscall %d\n", callno);
		    err = ENOSYS;
		    break;
	}


	if (err) {
		/*
		 * Return the error code. This gets converted at
		 * userlevel to a return value of -1 and the error
		 * code in errno.
		 */
		tf->tf_v0 = err;
		tf->tf_a3 = 1;      /* signal an error */
	}
	else {
		/* Success. */
		tf->tf_v0 = retval;
		tf->tf_a3 = 0;      /* signal no error */
	}
	
	/*
	 * Now, advance the program counter, to avoid restarting
	 * the syscall over and over again.
	 */
	
	tf->tf_epc += 4;

	/* Make sure the syscall code didn't forget to lower spl */
	assert(curspl==0);
}

void
md_forkentry(struct trapframe *tf)
{
	/*
	 * This function is provided as a reminder. You need to write
	 * both it and the code that calls it.
	 *
	 * Thus, you can trash it and do things another way if you prefer.
	 */

	(void)tf;
}
