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
 * @(#)builtin.c: tcp port multiplexter service
 * $Id: tcpmux.c,v 1.2 2002/02/07 02:09:00 cvs Exp $
 */

#include <inetd.h>
#include <inetd_int.h>
#include "misc.h"

#define strwrite(fd, buf)	(void)send(fd, buf, sizeof (buf) - 1, 0)

/* TCP port service Multiplexer (TCPMUX) */
struct inetd_serv *tcpmux(SOCKET s) {
    struct inetd_serv *sep;
    char service[INETD_SERV_LEN+1];
    int len;
    struct hostent *hn;
    struct sockaddr_in from;
    int fromlen = sizeof (from);
    
    /* Get requested service name */
    if ((len = inetd_net_getline(s, service, INETD_SERV_LEN)) < 0) {
        strwrite(s, "-Error reading service name\r\n");
        return (NULL);
    }
    service[len] = '\0';
    
    if (getpeername(s, (struct sockaddr *)&from, &fromlen))
        inetd_log_message(INETD_LOG_WARN, "Service %s (%s): UNKNOWN client.",
                          "tcpmux", "inetd builtin");
    else
        inetd_log_message(INETD_LOG_INFO, "Service %s (%s)£º %s:%d",
                          "tcpmux", "inetd builtin",
                          (hn = gethostbyaddr((char *)&from.sin_addr,
                          sizeof (struct in_addr), from.sin_family)) ?
                          hn->h_name : inet_ntoa(from.sin_addr),
                          ntohs(from.sin_port));
    /*
     * Help is a required command, and lists available services,
     * one per line.
     */
    if (!stricmp(service, "help")) {
        for (sep = inetd_serv_tab; sep; sep = sep->is_next) {
            if (!INETD_ISMUX(sep))
                continue;
            (void)send(s, sep->is_service, strlen(sep->is_service), 0);
            strwrite(s, "\r\n");
        }
        return (NULL);
    }
    
    /* Try matching a service in inetd.conf with the request */
    for (sep = inetd_serv_tab; sep; sep = sep->is_next) {
        if (!INETD_ISMUX(sep))
            continue;
        if (!stricmp(service, sep->is_service)) {
            if (INETD_ISMUXP(sep)) {
                strwrite(s, "+Go\r\n");
            }
            return (sep);
        }
    }
    strwrite(s, "-Service not available\r\n");
    return (NULL);
}
