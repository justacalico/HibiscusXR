/*
 * The sdm845 libcutils does not export these two symbols (they are
 * versioned-private on this build). Tracing is best-effort, so return
 * an empty tag mask and no-op init.
 */
#include <stdint.h>

uint64_t atrace_get_enabled_tags(void);
void atrace_init(int fd);

uint64_t
atrace_get_enabled_tags(void)
{
   return 0;
}

void
atrace_init(int fd)
{
   (void)fd;
}
