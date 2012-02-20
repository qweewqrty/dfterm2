/* Shamelessly copied somewhere from FreeBSD. 
 * Removed irrelevant parts in regard to dfterm2. 
 * 
 * Also added logging to dfterm2 directly.
 */

/*-
 * Copyright (c) 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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

#include "logger.hpp"

extern "C"
{

int
openpty(int *amaster, int *aslave, char *name, struct termios *termp,
    struct winsize *winp)
{
	int master, slave, result;

    master = open("/dev/ptmx", O_RDWR|O_NOCTTY);
	//master = posix_openpt(O_RDWR|O_NOCTTY);
	if (master == -1)
    {
        LOG(dfterm::Error, "open(\"/dev/ptmx\", O_RDWR|O_NOCTTY) failed with errno == " << errno);
		return (-1);
    }

	if (grantpt(master) == -1)
    {
        LOG(dfterm::Error, "grantpt(" << master << ") failed with errno == " << errno);
		goto bad;
    }

	if (unlockpt(master) == -1)
    {
        LOG(dfterm::Error, "unlockpt(" << master << ") failed with errno == " << errno);
		goto bad;
    }

#ifdef __FreeBSD__
    char* buf;
    buf = ptsname(master);
    if (!buf)
    {
        LOG(dfterm::Error, "ptsname(" << master << ") failed with errno == "
                << errno);
        goto bad;
    }
#else
    char buf[1001];
    memset(buf, 0, 1001);
	result = ptsname_r(master, buf, 1000);
	if (result)
    {
        LOG(dfterm::Error, "ptsname_r(" << master << ", " << buf << ", 1000) failed with errno == " << errno);
		goto bad;
    }
#endif

	slave = open(buf, O_RDWR);
	if (slave == -1)
    {
        LOG(dfterm::Error, "open(" << buf << ", O_RDWR) failed with errno == " << errno);
		goto bad;
    }

	*amaster = master;
	*aslave = slave;

	if (termp)
		tcsetattr(slave, TCSAFLUSH, termp);
	if (winp)
		ioctl(slave, TIOCSWINSZ, (char *)winp);

	return (0);

bad:	
    close(master);
	return (-1);
}

/* Never writes to 'name', even if its not null */
int
forkpty(int *amaster, char *name, struct termios *termp, struct winsize *winp)
{
	int master, slave, pid, result;

	if (openpty(&master, &slave, name, termp, winp) == -1)
		return (-1);
	switch (pid = fork()) {
	case -1:
    {
        LOG(dfterm::Error, "fork() failed with errno == " << errno);
		return (-1);
    }
	case 0:
		/*
		 * child
		 */
		(void) close(master);
        (void) setsid();
        if (ioctl(slave, TIOCSCTTY, (char *)NULL) != -1)
        {
            (void) dup2(slave, 0);
            (void) dup2(slave, 1);
            (void) dup2(slave, 2);
            if (slave > 2)
                (void) close(slave);
        }
        return (0);
	}
	/*
	 * parent
	 */
	*amaster = master;
	(void) close(slave);
	return (pid);
}

} // extern "C"

