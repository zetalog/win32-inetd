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
 * @(#)net.c: inetd winsock routines
 * $Id: net.c,v 1.5 2002/02/05 05:16:09 zlv Exp $
 */

#include <inetd.h>
#include <inetd_int.h>

/* Initialize winsock dll
 */
void inetd_net_init() {
    WSADATA wsa;
    if ((WSAStartup(MAKEWORD(2, 0), &wsa) != 0) || (wsa.wVersion < 1)) {
        inetd_log_message(INETD_LOG_FAILURE, "inetd_net_init: WSAStartup - %m");
        exit(-1);
    }
}

/* Finalize winsock dll
 */
void inetd_net_fin() {
    if (WSACleanup()) {
        inetd_log_message(INETD_LOG_WARN, "inetd_net_fin: WSACleanup - %m");
    }
}

/* Wrapper of closesocket
 * Accepts: * pointer to socket descriptor
 */
void inetd_net_close(SOCKET *fdp) {
    LINGER linger = { 1, 0 };
    setsockopt(*fdp, SOL_SOCKET, SO_LINGER, (LPCTSTR)&linger, sizeof (linger));
    shutdown(*fdp, SD_BOTH);
    if (closesocket(*fdp));
        /*inetd_log_message(INETD_LOG_WARN, "closesocket %d: %m", *fdp);*/
    (*fdp) = INVALID_SOCKET;
}

int inetd_net_recv(SOCKET fd, char *buf, int len) {
    int n = 0;
    fd_set readable;
    struct timeval tv = { INETD_WAIT_SEC, 0 };

    do {
        FD_ZERO(&readable);
        FD_SET(fd, &readable);
        if ((n = select(fd + 1, &readable, (fd_set *)0, (fd_set *)0, &tv)) < 0) {
            return SOCKET_ERROR;
        } else if (n > 0) {
            return recv(fd, buf, len, 0);
        }
    } while (!inetd_sess_end);
    return (n);
}

int inetd_net_recvfrom(SOCKET fd, char *buf, int len, struct sockaddr *from, int *fromlen) {
    int n = 0;
    fd_set readable;
    struct timeval tv = { INETD_WAIT_SEC, 0 };

    do {
        FD_ZERO(&readable);
        FD_SET(fd, &readable);
        if ((n = select(fd + 1, &readable, (fd_set *)0, (fd_set *)0, &tv)) < 0) {
            return SOCKET_ERROR;
        } else if (n > 0) {
            return recvfrom(fd, buf, len, 0, from, fromlen);
        }
    } while (!inetd_sess_end);
    return (n);
}

int inetd_net_getline(SOCKET fd, char *buf, int len) {
    int count = 0, n;
    
    do {
        n = inetd_net_recv(fd, buf, len-count);
        if (n == 0)
            return (count);
        if (n < 0)
            return (-1);
        while (--n >= 0) {
            if (*buf == '\r' || *buf == '\n' || *buf == '\0')
                return (count);
            count++;
            buf++;
        }
    } while (count < len);
    return (count);
}
