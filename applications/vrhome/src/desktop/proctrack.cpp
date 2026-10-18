#include "proctrack.h"

#include <sys/wait.h>
#include <cerrno>
#include <signal.h>
#include <unistd.h>

bool spawnGone(pid_t pid) {
    if (pid <= 0) return true;
    int st;
    const pid_t r = waitpid(pid, &st, WNOHANG);
    if (r == 0) return false;
    if (r < 0 && errno != ECHILD) return false;
    // pid reaped or never ours: the process group can still hold a
    // wrapper's children - only report gone once that is empty too
    return kill(-pid, 0) != 0 && errno != EPERM;
}
