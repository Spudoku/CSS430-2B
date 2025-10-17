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

  int slices[3]; // slice[i] means that level i's current time slice
  int maxSlices[] = {1, 2, 4};
  for (int i = 0; i < 3; i++)
    slices[i] = 0; // all levels start slice 0.

  while (true)
  {
    int level = -1;

    for (int i = 0; i < 3; i++)
    {
      if (!queue[i].empty())
      {
        level = i;
        break;
      }
    }

    if (level != -1)
    {
      current = queue[level].front();
      queue[level].pop();
    }
    else
    {
      break; // no more processes to terminate scheduler
    }

    // check if current is alive
    if (kill(current, 0) == 0)
    { // current process is alive
      cerr << "\nscheduler: resumed " << current << endl;
      kill(current, SIGCONT); // run it for a next quantum
      schedulerSleep();
      kill(current, SIGSTOP);
    }

    if (kill(current, 0) == 0)
    {
      if (level == 1 || level == 2)
      {
        slices[level]++;
      }

      if (slices[level] > maxSlices[level])
      {
        queue[level].pop();
        slices[level] = 0;

        if (level != 2)
        {
          queue[level + 1].push(current);
        }
        else
        {
          queue[2].push(current);
        }
      }
      else
      {
      }
    }
    else
    { // current process is dead
      cerr << "scheduler: confirmed " << current << "'s termination" << endl;
      slices[level] = 0;
    }
  }
  cerr << "scheduler: has no more process to run" << endl;
}

// Textbook references (from index):
// "multilevel feedback-queue scheduling algorithm: 216-217"
// "multilevel queue schedulinig algorith": 214-216
//
