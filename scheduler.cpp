#include <signal.h> // kill
#include <unistd.h> // usleep
#include <iostream> // cout, cerr, endl
#include "scheduler.h"

Scheduler::Scheduler(int quantum)
{
  msec = (quantum > 0) ? quantum : DEFAULT_QUANTUM;
}

bool Scheduler::addProcess(int pid)
{
  if (kill(pid, SIGSTOP) != 0)
  {
    cerr << "process[" << pid << "] can't be paused." << endl;
    return false;
  }
  queue[0].push(pid); // enqueue this process
  return true;
}

void Scheduler::schedulerSleep()
{
  usleep(msec);
  cerr << "scheduler: completed " << (++nQuantums) << " quantums" << endl;
}

void Scheduler::run_rr()
{
  cerr << "scheduler (round robin): started" << endl;
  int current = 0;

  while (true)
  {
    if (queue[0].size() == 0) // no more processes to terminate scheduler
      break;
    current = queue[0].front(); // pick up the first process from the queue

    if (kill(current, 0) == 0)
    { // current process is alive
      cerr << "\nscheduler: resumed " << current << endl;
      kill(current, SIGCONT); // run it for a next quantum
      schedulerSleep();
      kill(current, SIGSTOP);
    }

    queue[0].pop();
    if (kill(current, 0) == 0)
    { // current process is still alive
      queue[0].push(current);
    }
    else
    { // current process is dead
      cerr << "scheduler: confirmed " << current << "'s termination" << endl;
      current = 0;
    }
  }
  cerr << "scheduler: has no more process to run" << endl;
}

void Scheduler::run_mfq()
{
  cerr << "scheduler (multilevel feedback queue): started" << endl;
  int current = 0;  // current pid
  int previous = 0; // previous pid

  int slices[3]; // slice[i] means that level i's current time slice.
  for (int i = 0; i < 3; i++)
    slices[i] = 0; // all levels start slice 0.

  while (!queue[0].empty() || !queue[1].empty() || !queue[2].empty())
  {
    int level = 0;
    for (; level < 3; level++)
    {
      // if the current level's slice is 0.
      if (slices[0] == 0)
      {
        // check the current level queue is empty.
        if (queue[level].empty())
        {
          // if so, go to a next lower level queue.
          level++;
        }
        else
        {
          // otherwise, pick up a pid from this queue
          current = queue[level].front();
          queue[level].pop();
        }
      }
      else
      {
        // if the current level's slice is 1, 2, or 3, the previous process should run continuously.
      }
    }
    // if we reached level 3, (i.e., the lowest level) and found no processes to schedule
    if (queue[0].empty() && queue[1].empty() && queue[2].empty())
    {
      // finish scheduler.cpp
      break;
    }

    // check if a process to run is still active.
    // if so, resumt it, calls schedulerSleep( ) to give a time quantum.
    // then, suspends it.
    if (kill(current, 0) == 0)
    { // current process is alive
      cerr << "\nscheduler: resumed " << current << endl;
      kill(current, SIGCONT); // run it for a next quantum
      schedulerSleep();
      kill(current, SIGSTOP);
    }

    // check if this process is still active.

    if (kill(current, 0) == 0)
    {
      queue[level].push(current);
      if (level == 1 || level == 2)
      {
        // if so and if the current level is 1 or 2, shift to a next slice
        slices[level]++;
      }

      // if the next slice was wrapped back to 0. this pid should
      if (slices[level] > 3)
      {
        slices[level] = 0;
        if (level == 2)
        {
          // go back to the lowest level queue
          level = 0;
        }
        else
        {
          // go to the next level queue or
          level++;
        }
      }
    }
    else
    {
      // current process is dead, print out:
      cerr << "scheduler: confirmed " << current << "'s termination" << endl;
    }
  }
  cerr << "scheduler: has no more process to run" << endl;
}

// Textbook references (from index):
// "multilevel feedback-queue scheduling algorithm: 216-217"
// "multilevel queue schedulinig algorith": 214-216
//
