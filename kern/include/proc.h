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

/* Get a pointer to the current process' parent */
struct process *process_getparent(void);

/* Call once during startup. */
struct process *process_bootstrap(void);

#endif /* _PROC_H_ */
