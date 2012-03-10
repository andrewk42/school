#ifndef _PROC_H_
#define _PROC_H_

#include <machine/trapframe.h>

struct array;

/*
 * Process table.
 */

struct proc_table {
    u_int32_t max_processes;
    struct array *proc_list;
    pid_t nextpid;
};


/*
 * Definition of a process.
 */

struct thread;

struct process {
    pid_t p_id, p_parent;
	struct thread *p_thread;
};

/* Call once during startup. */
struct process *process_bootstrap(void);

/*
 * Make a new thread, which will start executing at "func".  The
 * "data" arguments (one pointer, one integer) are passed to the
 * function.  The current thread is used as a prototype for creating
 * the new one. If "ret" is non-null, the thread structure for the new
 * thread is handed back. (Note that using said thread structure from
 * the parent thread should be done only with caution, because in
 * general the child thread might exit at any time.) Returns an error
 * code.
 */
int process_fork(const char *name, 
		void *data1, unsigned long data2, 
		void (*func)(void *, unsigned long),
		struct process **ret);

/* Machine dependent entry point for new threads. */
void md_forkentry(struct trapframe *tf);

#endif /* _PROC_H_ */
