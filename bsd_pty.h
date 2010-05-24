#ifndef bsd_pty_h
#define bsd_pty_h

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

extern int openpty(int *amaster, int *aslave, char *name, struct termios *termp, struct winsize *winp);
extern int forkpty(int *amaster, char *name, struct termios *termp, struct winsize *winp);

#ifdef __cplusplus
}; /* extern "C" */
#endif

#endif

