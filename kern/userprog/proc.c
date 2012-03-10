/*
 * Process system.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/pcb.h>
#include <array.h>
#include <thread.h>
#include <proc.h>

/* Global variable for the process currently active at any given time. */
struct process *curproc;

/* System process table. */
static struct proc_table *proc_table;

/*
 * Table usage
 */
static
pid_t
request_pid(struct process *p) {
    int err;
    pid_t ret;

    /*
     * First check if we are at the point of re-using pids or not
     */
    if ((u_int32_t)array_getnum(proc_table->proc_list) == proc_table->max_processes) {
        /*
         * If here, the process table has reached its maximum size and we will
         * try to reuse a pid.
         */

        // Remember where we are starting from
        pid_t start = proc_table->nextpid;

        // Find the next available pid
        while (array_getguy(proc_table->proc_list, proc_table->nextpid) != NULL) {
            proc_table->nextpid = (proc_table->nextpid + 1) % proc_table->max_processes;

            // Check if we've looped around. If so, can't allocate anymore processes
            if (proc_table->nextpid == start) {
                /*
                 * Can't return EAGAIN without it being interpreted as a valid pid. So
                 * convention is to return -1 in place of EAGAIN, so that it can
                 * eventually be converted to EAGAIN in mips_syscall().
                 */
                return -1;
            }
        }

        /*
         * If here, the table index proc_table->nextpid is NULL and available.
         * Set the list's nextpid element to be the new process.
         */
        array_setguy(proc_table->proc_list, proc_table->nextpid, p);
    } else {
        /*
         * If here, we haven't wrapped around pids yet and just need to increment
         */

        // Attempt to preallocate space for 1 additional process in the table
        err = array_preallocate(proc_table->proc_list, proc_table->nextpid+1);

        /*
         * Can't return ENOMEM without it being interpreted as a valid pid. So
         * convention is to return -2 in place of ENOMEM, so that it can
         * eventually be converted to ENOMEM in mips_syscall().
         */
        if (err == ENOMEM) {
            return -2;
        }

        // Add the new process to the table
        array_add(proc_table->proc_list, p);
    }

    // Ensure the new process has the right index
    assert(array_getguy(proc_table->proc_list, proc_table->nextpid) == p);

    // Set the return value
    ret = proc_table->nextpid;

    // Increment the nextpid counter
    proc_table->nextpid++;

    return ret;
}

/*
 * Process initialization.
 */
struct process *
process_bootstrap(void)
{
    struct process *me;

    // Create the process table
	proc_table = kmalloc(sizeof(struct proc_table));
	if (proc_table == NULL) {
		panic("Cannot create process table\n");
	}

    // Create the actual process array list
    proc_table->proc_list = array_create();
	if (proc_table->proc_list == NULL) {
		panic("Cannot create process table\n");
	}

    /*
     * Calculate the limit on the maximum number of
     * active processes. Roughly estimate that in a
     * situation where we have many processes, each
     * one will have only 1 thread and require space
     * for the size of its stack. And to be safe,
     * allow up to 1 half of physical memory to be
     * used for process stacks.
     */
    proc_table->max_processes = mips_ramsize() / STACK_SIZE / 2;

    // Initialize next available pid of process table
    proc_table->nextpid = 1;

    /*
     * Done process table creation
     */

    // Create the first process
    me = kmalloc(sizeof(struct process));
    if (me == NULL) {
        panic("Cannot create the first process\n");
    }

    // Set the self and parent pids
    me->p_id = me->p_parent = 0;

    // Set up the thread
    me->p_thread = thread_bootstrap();

    // Add to the process table
    array_add(proc_table->proc_list, me);

    // Set curproc
    curproc = me;

    return me;
}

int
process_fork(const char *name, 
		void *data1, unsigned long data2, 
		void (*func)(void *, unsigned long),
		struct process **ret)
{
    /*
     * Get a pid from the process table. If this fails, the process table
     * is full, so return EAGAIN.
     */

    

    /*
     * Call thread_fork(), which is passed the thread's name, a function
     * to execute, and arguments for that function.f
     * 
     * thread_fork() specifically does the following:
     * - calls thread_create(), which attempts to allocate the new thread
     *   struct, and the thread's name (t_name). Also sets the new thread's
     *   other pointer members t_sleepaddr, t_stack, t_vmspace, and t_cwd
     *   to NULL.
     * - If thread_create() failed, thread_fork() returns ENOMEM
     * - Attempts to allocate the new thread's t_stack, returns ENOMEM on
     *   failure
     * - Has the new thread inherhit the same cwd as the current one, by
     *   calling VOP_INCREF(curthread->t_cwd) and shallow copying t_cwd to
     *   the new thread's t_cwd
     * - calls md_initpcb(), which does the following:
     *    - sets the new thread's pcb's pcb_switchstack and pcb_kstack
     *      pointers to specific places on t_stack
     *    - sets up the new thread's switchframe, which includes the
     *      function to be run after the transition of the new thread
     *      from mips_threadstart() -> mi_threadstart() -> function
     * - calls array_preallocate on the sleeper and zombie queues to
     *   increase by size 1, and returns ENOMEM if fail
     * - calls scheduler_preallocate to increase by size 1, and returns
     *   ENOMEM if fail
     * - calls make_runnable(), which adds the new thread to the run queue
     * - increases global numthread counter
     */

    // Create new process' address space from current process

    // Create the new process' first thread of execution

    // Create copies of other resources (e.g. open files, sockets, etc.)

    // Have the new thread (implementing the new process) run forkentry()
    //md_forkentry(tf);

    (void)name;
    (void)data1;
    (void)data2;
    (void)func;
    (void)ret;

    return 0;
}
