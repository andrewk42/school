/*
 * Synchronization primitives.
 * See synch.h for specifications of the functions.
 */

#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <curthread.h>
#include <machine/spl.h>

#include "opt-A1.h"

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *namearg, int initial_count)
{
	struct semaphore *sem;

	assert(initial_count >= 0);

	sem = kmalloc(sizeof(struct semaphore));
	if (sem == NULL) {
		return NULL;
	}

	sem->name = kstrdup(namearg);
	if (sem->name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->count = initial_count;
	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	spl = splhigh();
	assert(thread_hassleepers(sem)==0);
	splx(spl);

	/*
	 * Note: while someone could theoretically start sleeping on
	 * the semaphore after the above test but before we free it,
	 * if they're going to do that, they can just as easily wait
	 * a bit and start sleeping on the semaphore after it's been
	 * freed. Consequently, there's not a whole lot of point in 
	 * including the kfrees in the splhigh block, so we don't.
	 */

	kfree(sem->name);
	kfree(sem);
}

void 
P(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	assert(in_interrupt==0);

	spl = splhigh();
	while (sem->count==0) {
		thread_sleep(sem);
	}
	assert(sem->count>0);
	sem->count--;
	splx(spl);
}

void
V(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);
	spl = splhigh();
	sem->count++;
	assert(sem->count>0);
	thread_wakeup(sem);
	splx(spl);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(struct lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->name = kstrdup(name);
	if (lock->name == NULL) {
		kfree(lock);
		return NULL;
	}

#if OPT_A1
	lock->occupied = 0;
    lock->owner = NULL;
#else
    // add stuff here as needed
#endif // OPT_A1
	
	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);

#if OPT_A1
	// Ensure no one currently holds the lock
    assert(!lock->occupied && lock->owner == NULL);
#else
    // add stuff here as needed
#endif // OPT_A1
	
	kfree(lock->name);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
#if OPT_A1
    int spl;
	assert(lock != NULL);

    // As with semaphores, don't block in an interrupt handler
	assert(in_interrupt==0);

    // Maintain same structure as semaphore; ensure this is atomic/not interrupted
    spl = splhigh();
	while (lock->occupied && lock->owner != curthread) {
        // Wait if the lock is being held by another thread (allow re-acquisition)
		thread_sleep(lock);
	}

    assert(!lock->occupied || (lock->occupied && lock->owner == curthread));

    // Set the occupied flag and make the current thread the owner
    lock->occupied = 1;
	lock->owner = curthread;

    // Restore previous priority level
	splx(spl);
#else
    // Write this

    (void)lock; // suppress warning until code gets written
#endif // OPT_A1
}

void
lock_release(struct lock *lock)
{
#if OPT_A1
	int spl;
    assert(lock != NULL);

    // Make this atomic
    spl = splhigh();

    // Reset occupied flag and clear owner
    lock->occupied = 0;
    lock->owner = NULL;

    // Wake up a potential next owner
	thread_wakeup(lock);

    // Restore previous priority level
	splx(spl);
#else
    // Write this

    (void)lock; // suppress warning until code gets written
#endif // OPT_A1
}

int
lock_do_i_hold(struct lock *lock)
{
#if OPT_A1
	return lock->owner == curthread;
#else
    // Write this

    (void)lock; // suppress warning until code gets written

    return 1; // dummy until code gets written
#endif // OPT_A1
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(struct cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->name = kstrdup(name);
	if (cv->name==NULL) {
		kfree(cv);
		return NULL;
	}

#if OPT_A1
    // Initialize counter
    cv->count = 0;

    // Create queue
    cv->q = q_create(1);
#else
	// add stuff here as needed
#endif // OPT_A1
	
	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

#if OPT_A1
    // Ensure that we have no one waiting
    assert(cv->count == 0);

    // Ensure the queue is empty
    assert(q_empty(cv->q));

    // Destory the queue
    q_destroy(cv->q);
#else
	// add stuff here as needed
#endif // OPT_A1
	
	kfree(cv->name);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
#if OPT_A1
    int spl;

    // Ensure that both locks exist, and that we currently own the mutex lock
    assert(cv != NULL);
    assert(lock != NULL);
    assert(lock_do_i_hold(lock));

    // Don't block in an interrupt handler
	assert(in_interrupt==0);

    // Begin atomicity
    spl = splhigh();

    // Release the mutex lock
    lock_release(lock);

    // Increment waiting count
    cv->count++;

    // Expand the queue, if necessary
    assert(!q_preallocate(cv->q, cv->count));

    // Add this thread to the queue
    assert(!q_addtail(cv->q, curthread));

    /*
     * Put this thread to sleep. Have its addr key as a pointer to itself for 2 reasons:
     * 1) Ensure FIFO ordering for signalled threads.
     * 2) Ensure 1 thread is woken up at a time.
     */
    thread_sleep(curthread);

    // Once woken up, reacquire the same mutex lock (may sleep again)
    lock_acquire(lock);

    // Restore previous priority level
	splx(spl);
#else
	// Write this
	(void)cv;    // suppress warning until code gets written
	(void)lock;  // suppress warning until code gets written
#endif // OPT_A1
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
#if OPT_A1
    int spl;

    /* Ensure that both locks exist.
     * Not really sure why there is a pointer to a mutex lock
     * as an argument, is it to needlessly ensure all users of
     * this cv associate with the same mutex lock? Don't see
     * the point in that since every thread has its own stack */
    assert(cv != NULL);
    assert(lock != NULL);
    assert(lock_do_i_hold(lock));

    // Begin atomicity
    spl = splhigh();

    // Decrement waiting thread count
    cv->count--;

    // Dequeue next waiting thread
    struct thread *signalled = q_remhead(cv->q);
    
    // Wake up a thread
    thread_wakeup(signalled);

    // Restore previous priority level
	splx(spl);
#else
	// Write this
	(void)cv;    // suppress warning until code gets written
	(void)lock;  // suppress warning until code gets written
#endif // OPT_A1
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
#if OPT_A1
    assert(cv != NULL);
    assert(lock != NULL);
    assert(lock_do_i_hold(lock));

    // Just call signal() until all the threads are woken up
    while (cv->count > 0) {
        cv_signal(cv, lock);
    }
#else
	// Write this
	(void)cv;    // suppress warning until code gets written
	(void)lock;  // suppress warning until code gets written
#endif // OPT_A1
}
