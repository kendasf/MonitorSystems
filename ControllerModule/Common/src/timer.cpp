#include <unistd.h>
#include <sys/times.h>
#include "timer.h"
#include <chrono>

auto start_time = std::chrono::steady_clock::now();

unsigned long long GetTickCount() {
  std::chrono::duration<float> duration = std::chrono::steady_clock::now() - start_time;
  return duration.count() * 1000;
}

unsigned long long TicksElapsed(unsigned long long t1) {
  if (t1 == 0)
    return (long) 0;

  return (GetTickCount() - t1);
}
