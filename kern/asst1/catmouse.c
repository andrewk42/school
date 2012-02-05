/*
 * catmouse.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 * 26-11-2007: KMS : Modified to use cat_eat and mouse_eat
 * 21-04-2009: KMS : modified to use cat_sleep and mouse_sleep
 * 21-04-2009: KMS : added sem_destroy of CatMouseWait
 *
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>

#if OPT_A1
#include "kitchen.h"
#endif // OPT_A1

/*
 * 
 * cat,mouse,bowl simulation functions defined in bowls.c
 *
 * For Assignment 1, you should use these functions to
 *  make your cats and mice eat from the bowls.
 * 
 * You may *not* modify these functions in any way.
 * They are implemented in a separate file (bowls.c) to remind
 * you that you should not change them.
 *
 * For information about the behaviour and return values
 *  of these functions, see bowls.c
 *
 */

/* this must be called before any calls to cat_eat or mouse_eat */
extern int initialize_bowls(unsigned int bowlcount);

extern void cat_eat(unsigned int bowlnumber);
extern void mouse_eat(unsigned int bowlnumber);
extern void cat_sleep(void);
extern void mouse_sleep(void);

/*
 *
 * Problem parameters
 *
 * Values for these parameters are set by the main driver
 *  function, catmouse(), based on the problem parameters
 *  that are passed in from the kernel menu command or
 *  kernel command line.
 *
 * Once they have been set, you probably shouldn't be
 *  changing them.
 *
 *
 */
int NumBowls;  // number of food bowls
int NumCats;   // number of cats
int NumMice;   // number of mice
int NumLoops;  // number of times each cat and mouse should eat

/*
 * Once the main driver function (catmouse()) has created the cat and mouse
 * simulation threads, it uses this semaphore to block until all of the
 * cat and mouse simulations are finished.
 */
struct semaphore *CatMouseWait;

/*
 * 
 * Function Definitions
 * 
 */

#if OPT_A1
struct kitchen *k = NULL;  // global kitchen variable

struct kitchen *kitchen_create() {
    int i;

    // Create the top-level struct
    struct kitchen *k = kmalloc(sizeof(struct kitchen));

    if (k == NULL) {
        return NULL;
    }

    // Construct the bowl lock array
    k->bowl_locks = kmalloc(NumBowls * sizeof(struct lock *));
    if (k->bowl_locks == NULL) {
        kfree(k);
        return NULL;
    }

    // Construct each bowl lock
    for (i = 0; i < NumBowls; i++) {
        k->bowl_locks[i] = lock_create("bowl");
    }

    // Construct the entrance lock
    k->kitchen_lock = lock_create("enter");

    // Construct the kitchen cv
    k->kitchen_cv = cv_create("kitchen");

    // Construct the group queue
    k->group_list = q_create(1);

    // Initialize the current_creature flag
    k->current_creature = 2;

    // Initialize the counter
    k->creature_count = 0;

    return k;
}

void enter_kitchen(const int creature_type) {
    /*
     * Policy: Let any amount of cats try to access any amount of bowls at once,
     * or let any amount of mice try to access any amount of bowls at once, but
     * never mix.
     *
     * So, if the kitchen is empty, the first creature can enter regardless. If
     * that creature is a mouse, let all subsequent creatures that are mice enter
     * without delay. But, as soon as the next creature is a cat, stop letting
     * all subsequent creatures in, and put them in groups waiting in line.
     * Idea is to wait until all the mice currently in the kitchen are done, then
     * let a "burst" of cats in, then wait until all of them are done, then let a
     * "burst" of mice in, etc.
     *
     * For example, if x's are mice, and y's are cats, say we have this stream of
     * animals (arrived at the kitchen in order from right to left)
     *
     *   Time   |   Outside of Kitchen    |    Inside Kitchen   |  Done eating
     *    t0       yyyxyyyyyxxxxxxyyyyxx
     *    t1       yyyxyyyyyxxxxxxyyyy               xx
     *    t2       yyyxyyyyyxxxxxx                   yyyy               xx
     *    t3       yyyxyyyyy                         xxxxxx         yyyyxx
     *
     *
     * This policy has the objective of being as fair as possible, with efficieny
     * as an important but secondary objective (we let any amount of 1 creature
     * type in at once). Besides, we are multithreading to get a huge efficiency
     * gain in the first place.
     */

    // Want mutual exclusion for group processing
    lock_acquire(k->kitchen_lock);

    // First creature in gets to initially set the current_creature flag and enter
    if (k->current_creature == 2) {
        k->current_creature = creature_type;
    }
    /* 
     * If the queue is not empty, know that there are other groups to go first.
     * Don't barge ahead of another group even if you match the current creature.
     */
    else if (!q_empty(k->group_list)) {
        // Inspect the last group in line
        int index = q_getend(k->group_list) > 0 ? q_getend(k->group_list)-1 : q_getsize(k->group_list) - 1;
        struct kgroup *g = (struct kgroup *)q_getguy(k->group_list, index);

        if (g->type == creature_type) {
            // If the last group is of your type, merge into that group
            g->amount++;
        } else {
            // Otherwise, start a new last group
            g = kmalloc(sizeof(struct kgroup));
            g->type = creature_type;
            g->amount = 1;

            // Enqueue new group
            q_addtail(k->group_list, g);
        }

        // Wait
        cv_wait(k->kitchen_cv, k->kitchen_lock);
    }
    /*
     * If we aren't the first creature, and the queue is empty, check if we match the
     * creature currently in the kitchen. If not, we are going to start the only group
     * in the queue.
     */
    else if (k->current_creature != creature_type) {
        // Create a group struct
        struct kgroup *g = kmalloc(sizeof(struct kgroup));

        // Group is of your type with 1 creature so far
        g->type = creature_type;
        g->amount = 1;

        // Enqueue new group
        q_addtail(k->group_list, g);

        // Wait
        cv_wait(k->kitchen_cv, k->kitchen_lock);
    }

    // If here, we have been granted access to the kitchen

    // Set the creature type currently owning the kitchen
    k->current_creature = creature_type;

    // Increment creature count
    k->creature_count++;

    /*
     * Release kitchen lock before accessing the bowls, because we want any amount
     * of one type of creature to be able to access the bowls at once.
     */
    lock_release(k->kitchen_lock);
}

void exit_kitchen() {
    // Reacquire entrance lock
    lock_acquire(k->kitchen_lock);

    // Decrement count
    k->creature_count--;

    /*
     * If there are no creatures left, let in the next group waiting.
     * If there is no other group waiting, reset the switch that
     * indicates which creature type currently owns the kitchen.
     */
    if (!q_empty(k->group_list) && k->creature_count == 0) {
        // Dequeue first group in line
        struct kgroup *g = q_remhead(k->group_list);
        int i;

        // Signal every member of that group
        for (i = 0; i < g->amount; i++) {
            cv_signal(k->kitchen_cv, k->kitchen_lock);
        }

        // Destroy the group struct
        kfree(g);
    } else if (q_empty(k->group_list) && k->creature_count == 0) {
        // 2 is the "unset" value for k->current_creature
        k->current_creature = 2;
    }

    // Release enter lock and exit
    lock_release(k->kitchen_lock);
}

void eat(const int creature_type) {
    /*
     * Convention: creature_type=0 -> cat. creature_type=1 -> mouse.
     */

    // Try to enter kitchen and potentially get blocked/grouped
    enter_kitchen(creature_type);

    // Choose a random initial bowl index
    unsigned int bowl = ((unsigned int)random() % NumBowls);
    int i = 0;

    for (i = 0; i < NumBowls; i++) {
        // Try to acquire it, but don't block if it is occupied
        if (lock_tryacquire(k->bowl_locks[bowl])) break;

        // Try another bowl
        bowl = (bowl+1) % NumBowls;
    }

    // If all the bowls are occupied
    if (i == NumBowls) {
        // Just wait to acquire the initially chosen one
        bowl %= NumBowls;
        lock_acquire(k->bowl_locks[bowl]);
    }

    assert(lock_do_i_hold(k->bowl_locks[bowl]));

    // Eat
    if (creature_type) {
        mouse_eat(bowl+1);
    } else {
        cat_eat(bowl+1);
    }

    // Release this bowl's lock
    lock_release(k->bowl_locks[bowl]);

    exit_kitchen();
}

void kitchen_destroy(struct kitchen *k) {
    int i;

    // Destroy the queue elements
    while (!q_empty(k->group_list)) {
        kfree(q_remhead(k->group_list));
    }

    // Destroy the queue
    q_destroy(k->group_list);

    // Destroy the cv
    cv_destroy(k->kitchen_cv);

    // Destroy the entrance lock
    lock_destroy(k->kitchen_lock);

    // Destroy the bowl locks
    for (i = 0; i < NumBowls; i++) {
        lock_destroy(k->bowl_locks[i]);
    }

    // Destroy the bowl lock array
    kfree(k->bowl_locks);

    // Destroy the kitchen
    kfree(k);

    // Clear the pointer
    k = NULL;
}

#endif //OPT_A1

/*
 * cat_simulation()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds cat identifier from 0 to NumCats-1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Each cat simulation thread runs this function.
 *
 *      You need to finish implementing this function using 
 *      the OS161 synchronization primitives as indicated
 *      in the assignment description
 */

static
void
cat_simulation(void * unusedpointer, 
	       unsigned long catnumber)
{
  int i;
  unsigned int bowl;

  /* avoid unused variable warnings. */
  (void) unusedpointer;
  (void) catnumber;


  /* your simulated cat must iterate NumLoops times,
   *  sleeping (by calling cat_sleep() and eating
   *  (by calling cat_eat()) on each iteration */
  for(i=0;i<NumLoops;i++) {

    /* do not synchronize calls to cat_sleep().
       Any number of cats (and mice) should be able
       sleep at the same time. */
    cat_sleep();

    /* for now, this cat chooses a random bowl from
     * which to eat, and it is not synchronized with
     * other cats and mice.
     *
     * you will probably want to control which bowl this
     * cat eats from, and you will need to provide 
     * synchronization so that the cat does not violate
     * the rules when it eats */

#if OPT_A1
    (void)bowl; //suppress warning
    eat(0);
#else
    /* legal bowl numbers range from 1 to NumBowls */
    bowl = ((unsigned int)random() % NumBowls) + 1;
    cat_eat(bowl);
#endif // OPT_A1

  }

  /* indicate that this cat simulation is finished */
  V(CatMouseWait); 
}
	
/*
 * mouse_simulation()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds mouse identifier from 0 to NumMice-1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      each mouse simulation thread runs this function
 *
 *      You need to finish implementing this function using 
 *      the OS161 synchronization primitives as indicated
 *      in the assignment description
 *
 */

static
void
mouse_simulation(void * unusedpointer,
          unsigned long mousenumber)
{
  int i;
  unsigned int bowl;

  /* Avoid unused variable warnings. */
  (void) unusedpointer;
  (void) mousenumber;


  /* your simulated mouse must iterate NumLoops times,
   *  sleeping (by calling mouse_sleep()) and eating
   *  (by calling mouse_eat()) on each iteration */
  for(i=0;i<NumLoops;i++) {

    /* do not synchronize calls to mouse_sleep().
       Any number of mice (and cats) should be able
       sleep at the same time. */
    mouse_sleep();

    /* for now, this mouse chooses a random bowl from
     * which to eat, and it is not synchronized with
     * other cats and mice.
     *
     * you will probably want to control which bowl this
     * mouse eats from, and you will need to provide 
     * synchronization so that the mouse does not violate
     * the rules when it eats */

#if OPT_A1
    (void)bowl; // suppress warning
    eat(1);
#else
    /* legal bowl numbers range from 1 to NumBowls */
    bowl = ((unsigned int)random() % NumBowls) + 1;
    mouse_eat(bowl);
#endif // OPT_A1

  }

  /* indicate that this mouse is finished */
  V(CatMouseWait); 
}


/*
 * catmouse()
 *
 * Arguments:
 *      int nargs: should be 5
 *      char ** args: args[1] = number of food bowls
 *                    args[2] = number of cats
 *                    args[3] = number of mice
 *                    args[4] = number of loops
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up cat_simulation() and
 *      mouse_simulation() threads.
 *      You may need to modify this function, e.g., to
 *      initialize synchronization primitives used
 *      by the cat and mouse threads.
 *      
 *      However, you should should ensure that this function
 *      continues to create the appropriate numbers of
 *      cat and mouse threads, to initialize the simulation,
 *      and to wait for all cats and mice to finish.
 */

int
catmouse(int nargs,
	 char ** args)
{
  int index, error;
  int i;

  /* check and process command line arguments */
  if (nargs != 5) {
    kprintf("Usage: <command> NUM_BOWLS NUM_CATS NUM_MICE NUM_LOOPS\n");
    return 1;  // return failure indication
  }

  /* check the problem parameters, and set the global variables */
  NumBowls = atoi(args[1]);
  if (NumBowls <= 0) {
    kprintf("catmouse: invalid number of bowls: %d\n",NumBowls);
    return 1;
  }
  NumCats = atoi(args[2]);
  if (NumCats < 0) {
    kprintf("catmouse: invalid number of cats: %d\n",NumCats);
    return 1;
  }
  NumMice = atoi(args[3]);
  if (NumMice < 0) {
    kprintf("catmouse: invalid number of mice: %d\n",NumMice);
    return 1;
  }
  NumLoops = atoi(args[4]);
  if (NumLoops <= 0) {
    kprintf("catmouse: invalid number of loops: %d\n",NumLoops);
    return 1;
  }
  kprintf("Using %d bowls, %d cats, and %d mice. Looping %d times.\n",
          NumBowls,NumCats,NumMice,NumLoops);

  /* create the semaphore that is used to make the main thread
     wait for all of the cats and mice to finish */
  CatMouseWait = sem_create("CatMouseWait",0);
  if (CatMouseWait == NULL) {
    panic("catmouse: could not create semaphore\n");
  }

  /* 
   * initialize the bowls
   */
  if (initialize_bowls(NumBowls)) {
    panic("catmouse: error initializing bowls.\n");
  }

#if OPT_A1
  // Create the kitchen
  k = kitchen_create();
#endif

  /*
   * Start NumCats cat_simulation() threads.
   */
  for (index = 0; index < NumCats; index++) {
    error = thread_fork("cat_simulation thread",NULL,index,cat_simulation,NULL);
    if (error) {
      panic("cat_simulation: thread_fork failed: %s\n", strerror(error));
    }
  }

  /*
   * Start NumMice mouse_simulation() threads.
   */
  for (index = 0; index < NumMice; index++) {
    error = thread_fork("mouse_simulation thread",NULL,index,mouse_simulation,NULL);
    if (error) {
      panic("mouse_simulation: thread_fork failed: %s\n",strerror(error));
    }
  }

  /* wait for all of the cats and mice to finish before
     terminating */  
  for(i=0;i<(NumCats+NumMice);i++) {
    P(CatMouseWait);
  }

#if OPT_A1
  // Cleanup the kitchen lol
  kitchen_destroy(k);
#endif

  /* clean up the semaphore the we created */
  sem_destroy(CatMouseWait);

  return 0;
}

/*
 * End of catmouse.c
 */
