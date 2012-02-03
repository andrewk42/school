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
struct Kitchen {
    struct lock **bowlLocks;    // lock for each bowl
};

struct Kitchen *kitchen_create() {
    int i;
    struct Kitchen *k = kmalloc(sizeof(struct Kitchen));

    if (k == NULL) {
        return NULL;
    }

    k->bowlLocks = kmalloc(NumBowls * sizeof(struct lock *));
    if (k->bowlLocks == NULL) {
        kfree(k);
        return NULL;
    }

    for (i = 0; i < NumBowls; i++) {
        k->bowlLocks[i] = lock_create("bowl");
        assert(k->bowlLocks[i] != NULL);
    }

    return k;
}

void eat(struct Kitchen *k, int creature_type) {
    /*
     * Convention: creature_type=0 -> cat. creature_type=1 -> mouse.
     */

    // Choose random bowl
    unsigned int bowl = ((unsigned int)random() % NumBowls) + 1;

    // Acquire this bowl's lock
    lock_acquire(k->bowlLocks[bowl-1]);
    assert(lock_do_i_hold(k->bowlLocks[bowl-1]));

    // Eat
    if (creature_type) {
        mouse_eat(bowl);
    } else {
        cat_eat(bowl);
    }

    // Release this bowl's lock
    lock_release(k->bowlLocks[bowl-1]);
}

void kitchen_destroy(struct Kitchen *k) {
    int i;

    // Destroy the bowl locks
    for (i = 0; i < NumBowls; i++) {
        lock_destroy(k->bowlLocks[i]);
    }

    // Destroy the bowl lock array
    kfree(k->bowlLocks);

    // Destroy the kitchen
    kfree(k);
}

struct Kitchen *k;  // global kitchen variable
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
    eat(k, 0);
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
    eat(k, 1);
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
