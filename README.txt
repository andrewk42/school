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


Miscellaneous Comments:
-----------------------
In all of this repository, I follow variable naming conventions that
seemed to exist in OS-161 before I began working on it. So you will
notice lowercase struct types, underscores in function names, etc. that
I do not usually follow, but used in this scenario for consistency.

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
