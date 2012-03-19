####################################################################
########### README file for andrewk42's OS-161 project #############
####################################################################

This repository contains all of my work done on OS-161 in the context
of the University of Waterloo's CS350 Operating Systems course. All
of my resources came from the following course link:

http://www.student.cs.uwaterloo.ca/~cs350/W12/

All of the code here is written in C. The folder structure is
maintained in a way such that it is clear which files in the OS-161
directory I have edited. If you wish to run my files, you may do so
by following the instructions in the course link and plugging my
files into your OS-161 root before compiling.

Current Status:
---------------
-Locks and the cat/mouse simulation are complete.
-Part of an initial design for the Process has been committed, but it
 is not complete. The context switch sections need to be changed to
 account for Process objects, and this will be done in parallel with
 the completion of fork().
-An inital design for the fork() system call has been committed, but
 not tested and probably do not quite work yet. I have put this system
 call on hold because the programs that test it rely on other system
 calls to be implemented first.
-I am currently working on _exit(), then I plan on implementing write()
 directly after.


Style/Naming Conventions:
-------------------------
In all of this repository, I follow variable naming conventions that
seemed to exist in OS-161 before I began working on it. So you will
notice lowercase struct types, underscores in function names, etc. that
I do not usually follow, but used in this scenario for consistency.


Fork() System Call:
-------------------

/man/syscall/fork.html: Contains the html manpage for the fork()
system call. Considered to be the main specification for my design.
I did not change this file, but it is included in my commits due to
their relevance.

/include/unistd.h: Contains declarations for all the user-level system
call interfaces. I did not change this file either.

/kern/conf/conf.kern: A configuration file used before "make" to
indicate which files to include for particular assignments, and define
preprocessor constants so that the #if OPT_A[n] blocks work properly.

/kern/include/thread.h, /kern/thread/thread.c: Contain declarations
and definitions for Threads, which are important for fork() since the
calling thread has to be duplicated. In order to properly implement
Processes, I will have to modify these files to move the PCB/address
space/etc. into the Process object. But for now these are unchanged.

/kern/include/proc.h, /kern/userprog/proc.c: Contain declarations and
definitions for Processes, which are necssary for fork() since the
idea is to make a copy of the calling Process.

/kern/arch/mips/include/pcb.h, /kern/arch/mips/mips/pcb.c: Contain the
declarations and definitions for PCBs, which are relevant to fork().
These were not changed.

/kern/arch/mips/include/trapframe.h: Contains the trapframe stuct and
constants defining exception codes, which are relevant to fork(). These
were not changed.

/kern/include/kern/callno.h: Contains definitions of constants that
correspond to system calls. These wre not changed.

/kern/include/syscall.h, /kern/arch/mips/mips/syscall.c: Contain the
kernel-level system call interfaces, the mips_syscall() routine (which
is where the call number is read to branch to a particular kernel-level
system call), and the md_forkentry() routine (which is called as part
of the fork() system call).

/kern/main/main.c: Contains the boot() routine, which was modified to
call process_bootstrap() instead of thread_bootstrap() (threads are
now part of processes).


Multithreading/Concurrency Tools:
---------------------------------

/kern/include/synch.h, /kern/thread/synch.c: Contain my own
implementations of a normal (owner) lock and condition variable (lock)
that are used to ensure mutal exclusion and synchronization respectively
in the concurrent operation of the OS. Their design is heavily influenced
by the uOwnerLocks and uCondLocks in uC++.
(http://plg.uwaterloo.ca/usystem/uC++.html) Note that as per the
assignment 1 guidelines from the course link above, the code I wrote
is wrapped in #if OPT_A1 / #endif

/kern/asst1/catmouse.c: Contains a simulation not part of the actual
operation of OS-161, but proves as a good test for the owner/cond locks
implemented. The simulation describes a situation where there are bowls
of food to eat from in a common area, that cats and mice share. Since
these bowls are of close physical proximity, a mouse cannot be eating
from a bowl while a cat is eating from another bowl, and vice versa.
The goal of the simulation is to ensure that this rule is obeyed given
any amount of cats, mice, and bowls using locks. There were no constraints
on fairness and efficiency and these were left to the student to decide.
Here is a wordy description of my solution:
  * Let any amount of cats try to access any amount of bowls at once,
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
