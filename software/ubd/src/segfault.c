#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "segfault.h"

void segfault_sigaction(int signal, siginfo_t *si, void *arg)
{
    syslog(LOG_ERR, "Caught segfault at address %p\n", si->si_addr);
    abort();
}

void segfault_init(void)
{
    struct sigaction sa;

    setrlimit(RLIMIT_CORE,RLIM_INFINITY);

    memset(&sa, 0, sizeof(sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segfault_sigaction;
    sa.sa_flags   = SA_SIGINFO;

    sigaction(SIGSEGV, &sa, NULL);

}

