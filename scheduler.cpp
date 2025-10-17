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

      cerr << "level chosen: " << level << "; current chosen: " << current << "; slices[level]: " << slices[level] << endl;
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
    else
    {
      cerr << "scheduler: confirmed " << current << "'s termination" << endl;
      slices[level] = 0;
      queue[level].pop();
      continue;
    }

    // requeing and/or demotion
    if (kill(current, 0) == 0)
    {
      // process is still active
      slices[level]++;
      cerr << "\t current chosen: " << current << "; slices[level]: " << slices[level] << endl;
      // if maximum number of slices used, demote/requeue
      if (slices[level] >= maxSlices[level])
      {
        queue[level].pop();
        if (level == 2)
        {
          cerr << "\t" << current << " requeued at level 2" << endl;
          queue[2].push(current);
          slices[2] = 0;
        }
        else
        {
          cerr << "\t" << current << " demoted to level " << level + 1 << endl;
          queue[level + 1].push(current);
          // reset slices
          slices[level] = 0;
        }
      }
    }
    else
    { // current process is dead
      cerr << "scheduler: confirmed " << current << "'s termination" << endl;
      queue[level].pop();
      slices[level] = 0;
    }
  }
  cerr << "scheduler: has no more process to run" << endl;
}

// Textbook references (from index):
// "multilevel feedback-queue scheduling algorithm: 216-217"
// "multilevel queue schedulinig algorith": 214-216
//
