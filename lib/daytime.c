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
 * @(#)daytime.c: human-readable time service
 * $Id: daytime.c,v 1.2 2002/02/07 02:09:00 cvs Exp $
 */

#include <inetd.h>
#include <inetd_int.h>
#include "misc.h"
#include "error.h"
#include <time.h>

/* Return human-readable time of day */
void daytime_stream(SOCKET s, struct inetd_serv *sep) {
    char buffer[256];
    time_t clock;
    
    clock = time((time_t *) 0);
    
    (void)sprintf(buffer, "%.24s\r\n", ctime(&clock));
    (void)send(s, buffer, strlen(buffer), 0);
}

/* Return human-readable time of day */
void daytime_dgram(SOCKET s, struct inetd_serv *sep) {
    char buffer[256];
    time_t clock;
    struct sockaddr sa;
    int size;
    
    clock = time((time_t *) 0);
    
    size = sizeof (sa);
    if (inetd_net_recvfrom(s, buffer, sizeof (buffer), &sa, &size) < 0)
        return;
    (void)sprintf(buffer, "%.24s\r\n", ctime(&clock));
    (void)sendto(s, buffer, strlen(buffer), 0, &sa, sizeof (sa));
}
