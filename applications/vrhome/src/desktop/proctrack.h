#pragma once

#include <sys/types.h>

// true once the spawned pid is gone AND its process group is empty.
// reaps the child via waitpid(WNOHANG); spawn() setsid()s so the
// group id equals the spawn pid and outlives it while a wrapper's
// own children are still running
bool spawnGone(pid_t pid);
