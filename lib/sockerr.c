/*
 * Copyright (c) 2001
 * Prospect Information & Instrument Co. Ltd.  All rights reserved.
 * Author: Lv "Zetalog" Zheng
 * Internet: zl@prospect.com.cn
 *
 * Redistribution and use in source and binary forms in company, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the Prospect Information
 *    & Instrument Co, Ltd.
 * 3. Neither the name of the companay nor the names of its developer may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 4. Any out company redistribution are not permitted unless companay
 *    copyright no longer declaimed, but must not remove developer(s) above.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROSPECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROSPECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#)sockerr.c: winsock error convertion routines
 * $Id: sockerr.c,v 1.2 2002/01/21 11:20:08 zlv Exp $
 */

#include <stdio.h>
#include <winsock.h>

struct err_strs {
    char **strs;
    int first;
    int last;
};

static char *errs1[] = {
    /* EINTR            */ "Interrupted system call"
};

static char *errs2[] = {
    /* EBADF            */ "Bad file descriptor"
};

static char *errs3[] = {
    /* EACCES           */ "Permission denied",
    /* EFAULT           */ "Bad address"
};

static char *errs4[] = {
    /* EINVAL           */ "Invalid argument"
};

static char *errs5[] = {
    /* EMFILE           */ "Too many open files",
};

static char *errs6[] = {
    /* EWOULDBLOCK      */ "Resource temporarily unavailable",
    /* EINPROGRESS      */ "Operation now in progress",
    /* EALREADY         */ "Operation already in progress",
    /* ENOTSOCK         */ "Socket operation on non-socket",
    /* EDESTADDRREQ     */ "Destination address required",
    /* EMSGSIZE         */ "Message too long",
    /* EPROTOTYPE       */ "Protocol wrong type for socket",
    /* ENOPROTOOPT      */ "Protocol not available",
    /* EPROTONOSUPPORT  */ "Protocol not supported",
    /* ESOCKTNOSUPPORT  */ "Socket type not supported",
    /* EOPNOTSUPP       */ "Operation not supported on socket",
    /* EPFNOSUPPORT     */ "Protocol family not supported",
    /* EAFNOSUPPORT     */ "Address family not supported by protocol",
    /* EADDRINUSE       */ "Address already in use",
    /* EADDRNOTAVAIL    */ "Can't assign requested address",
    /* ENETDOWN         */ "Network is down",
    /* ENETUNREACH      */ "Network is unreachable",
    /* ENETRESET        */ "Network connection dropped on reset",
    /* ECONNABORTED     */ "Software caused connection abort",
    /* ECONNRESET       */ "Connection reset by peer",
    /* ENOBUFS          */ "No buffer space available",
    /* EISCONN          */ "Socket is already connected",
    /* ENOTCONN         */ "Socket is not connected",
    /* ESHUTDOWN        */ "Can't send after socket shutdown",
    /* ETOOMANYREFS     */ "Too many references: can't splice",
    /* ETIMEDOUT        */ "Connection timed out",
    /* ECONNREFUSED     */ "Connection refused",
    /* ELOOP            */ "Too many levels of symbolic links",
    /* ENAMETOOLONG     */ "File name too long",
    /* EHOSTDOWN        */ "Host is down",
    /* EHOSTUNREACH     */ "No route to host",
    /* ENOTEMPTY        */ "Directory not empty",
    /* EPROCLIM         */ "Too many processes",
    /* EUSERS           */ "Too many users",
    /* EDQUOT           */ "Disc quota exceeded",
    /* ESTALE           */ "Stale NFS file handle",
    /* EREMOTE          */ "Object is remote"
};

static char *errs7[] = {
    /* SYSNOTREADY      */ "Network subsystem unavailable",
    /* VERNOTSUPPORTED  */ "Requested WinSock version not supported",
    /* NOTINITIALISED   */ "WinSock was not initialized"
};

#ifdef WSAEDISCON
static char *errs8[] = {
    /* EDISCON          */ "Graceful shutdown in progress"
};
#endif

static char *errs9[] = {
    /* HOST_NOT_FOUND   */ "Unknown host",
    /* TRY_AGAIN        */ "Host name lookup failure",
    /* NO_RECOVERY      */ "Unknown server error",
    /* NO_DATA          */ "No address associated with name",
};

/* Some of these errors are defined in the winsock.h header file I have,
   but not in the Winsock 1.1 spec.  I include them some of them anyway,
   where it is not too hard to avoid referencing the symbolic constant. */

static struct err_strs sock_errlist[] = {
    { errs1,	WSAEINTR,	WSAEINTR },
    { errs2,	WSAEBADF,	WSAEBADF },
    { errs3,	WSAEACCES,	WSAEFAULT },
    { errs4,	WSAEINVAL,	WSAEINVAL },
    { errs5,	WSAEMFILE,	WSAEMFILE },
    { errs6,	WSAEWOULDBLOCK, WSAEHOSTUNREACH + 6 },
    { errs7,	WSASYSNOTREADY,	WSANOTINITIALISED },
#ifdef WSAEDISCON
    { errs8,	WSAEDISCON,	WSAEDISCON },
#endif
    { errs9,	WSAHOST_NOT_FOUND, WSANO_DATA }
};

char *sock_strerror(int errnum) {
    static char buf[40];
    int i;

    for (i = 0; i < (sizeof sock_errlist / sizeof *sock_errlist); i++) {
        if (errnum >= sock_errlist[i].first && errnum <= sock_errlist[i].last)
            return sock_errlist[i].strs[errnum - sock_errlist[i].first];
    }
    sprintf(buf, "Unknown socket error: %d", errnum);
    return buf;
}
