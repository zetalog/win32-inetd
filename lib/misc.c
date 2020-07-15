/*
 * Copyright (c) 2000
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
 * @(#)misc.c: inetd miscellaneous operation routines
 * $Id: misc.c,v 1.3 2002/01/25 09:18:55 cvs Exp $
 */

#include "misc.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <sys/timeb.h>

/* Get a block of free storage
 * Accepts: size of desired block
 * Returns: free storage block
 */
void *fsget(size_t size) {
    void *block = malloc(size ? size : (size_t)1);
    return (block);
}

/* Resize a block of free storage
 * Accepts: ** pointer to current block
 *	        new size
 */
void fsresize(void **block, size_t size) {
    *block = realloc(*block, size ? size : (size_t)1);
}

/* Return a block of free storage
 * Accepts: ** pointer to free storage block
 */
void fsgive(void **block) {
    free(*block);
    *block = 0;
}

/* Dup a string if null, dup an empty string
 * Accepts: * pointer to string to be dupped
 * Returns: * pointer to dupped string
 */
char *newstr(char *cp) {
    if (cp = _strdup(cp ? cp : ""))
        return (cp);
    exit(-1);
}

/* Posix gettimeofday() dummy
 * Accepts: * pointer to timeval indicating time since 1970
 *          * pointer to dummy time zone
 */
#ifndef HAVE_GETTIMEOFDAY
void gettimeofday(struct timeval *tv, void *dummy) {    
   struct _timeb timebuffer;
   _ftime(&timebuffer);
   tv->tv_sec = timebuffer.time;
   tv->tv_usec = timebuffer.millitm;
}
#endif /* HAVE_GETTIMEOFDAY */

void set_proc_title(char *a, int s) {
    int size;
    char *cp;
    struct sockaddr_in sin;
    char buf[80];
    
    cp = __argv[0];
    size = sizeof(sin);
    if (getpeername(s, (struct sockaddr *)&sin, &size) == 0)
        _snprintf(buf, sizeof (buf), "-%s [%s]", a, inet_ntoa(sin.sin_addr)); 
    else
        _snprintf(buf, sizeof (buf), "-%s", a); 
    strncpy(cp, buf, __argv[__argc-1] - cp);
    cp += strlen(cp);
    while (cp < __argv[__argc-1])
        *cp++ = ' ';
}
