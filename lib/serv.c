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
 * @(#)serv.c: inetd service related routines
 * $Id: serv.c,v 1.10 2002/02/20 07:53:45 zlv Exp $
 */

#include <inetd.h>
#include <inetd_int.h>
#include "misc.h"
#include "error.h"

/* externals */
struct inetd_serv *inetd_serv_tab;
SOCKET inetd_sock_max;
int inetd_sock_num;
fd_set inetd_sock_all;
int inetd_serv_toomany = INETD_TOOMANY;

#define sleep(x)        Sleep(1000*x)

/* internals */
struct servent *sp;

void inetd_serv_setup(struct inetd_serv *sep);
void inetd_serv_close(struct inetd_serv *sep);
void inetd_serv_retry();

/* Setup the service to the FD_SET
 * Accepts: * pointer to service table
 */
void inetd_serv_setup(struct inetd_serv *sep) {
    int on = 1;
    if (sep->is_up || sep->is_fd != INVALID_SOCKET)
        return;
    if ((sep->is_fd = socket(AF_INET, sep->is_socktype, 0)) < 0) {
        return;
    }

#ifdef SERVICE_REUSEADDR
#define	turnon(fd, opt) \
setsockopt(fd, SOL_SOCKET, opt, (char *)&on, sizeof (on))
    if (turnon(sep->is_fd, SO_REUSEADDR) < 0);
#undef turnon
#endif

    if (bind(sep->is_fd, (struct sockaddr *)&sep->is_ctrladdr, sizeof (sep->is_ctrladdr)) < 0) {
        inetd_net_close(&sep->is_fd);
        return;
    }
    if (sep->is_socktype == SOCK_STREAM)
        listen(sep->is_fd, 10);
    FD_SET(sep->is_fd, &inetd_sock_all);
    inetd_sock_num++;
    sep->is_up = 1;
    if (sep->is_fd > inetd_sock_max)
        inetd_sock_max = sep->is_fd;
}

/* Remove a service table
 * Accepts: * pointer to service table
 */
void inetd_serv_close(struct inetd_serv *sep) {
    if (sep->is_fd != INVALID_SOCKET) {
        inetd_sock_num--;
        FD_CLR(sep->is_fd, &inetd_sock_all);
        inetd_net_close(&sep->is_fd);
    }
    sep->is_count = 0;
    sep->is_up = 0;
    /*
     * Don't keep the pid of this running deamon: when inetd_serv_reap()
     * reaps this pid, it would erroneously increment inetd_sock_num.
     */
    if (sep->is_wait > 1)
        sep->is_wait = 1;
}

/* Retry inetd services
 */
void inetd_serv_retry() {
    struct inetd_serv *sep;
    for (sep = inetd_serv_tab; sep; sep = sep->is_next) {
        if (sep->is_fd == INVALID_SOCKET)
            inetd_serv_setup(sep);
    }
}

/* Config inetd services
 */
int inetd_serv_start() {
    struct inetd_serv *sep, *cp, **sepp;
    if (!inetd_conf_open(INETD_CONF_LOAD)) {
        inetd_log_message(INETD_LOG_ERROR, "inetd_serv_start: fopen - %m");
        return 0;
    }
    for (sep = inetd_serv_tab; sep; sep = sep->is_next) {
        sep->is_checked = 0;
        sep->is_up = 0;
    }
    while (cp = inetd_conf_load()) {
        for (sep = inetd_serv_tab; sep; sep = sep->is_next) {
            if (strcmp(sep->is_service, cp->is_service) == 0 &&
                strcmp(sep->is_proto, cp->is_proto) == 0)
                break;
        }
        if (sep != 0) { /* repetation */
#define SWAP(a, b) { char *c = a; a = b; b = c; }
            if (cp->is_user)
                SWAP(sep->is_user, cp->is_user);
            if (cp->is_server)
                SWAP(sep->is_server, cp->is_server);
            if (cp->is_cmdline)
                SWAP(sep->is_cmdline, cp->is_cmdline);
            inetd_conf_free(cp);
        } else {
            sep = inetd_conf_alloc(cp);
        }
        sep->is_checked = 1;
        if (INETD_ISMUX(sep)) {
            sep->is_fd = -1;
            continue;
        }
        sp = getservbyname(sep->is_service, sep->is_proto);
        if (sp == 0) {
            sep->is_checked = 0;
            continue;
        }
        if (sp->s_port != sep->is_ctrladdr.sin_port) {
            sep->is_ctrladdr.sin_family = AF_INET;
            sep->is_ctrladdr.sin_port = sp->s_port;
            if (sep->is_fd != INVALID_SOCKET)
                inetd_serv_close(sep);
        }
        if (sep->is_fd == INVALID_SOCKET)
            inetd_serv_setup(sep);
    }
    inetd_conf_close();
    sepp = &inetd_serv_tab;
    while (sep = *sepp) {
        if (sep->is_checked) {
            sepp = &sep->is_next;
            continue;
        }
        *sepp = sep->is_next;
        if (sep->is_fd != INVALID_SOCKET)
            inetd_serv_close(sep);
        inetd_conf_free(sep);
        fsgive((void **)&sep);
    }
    return 1;
}

/* inetd running step
 * Returns: running status
 */
int inetd_serv_step() {
    struct inetd_serv *sep;
    int n;
    SOCKET ctrl;
    fd_set readable;
    struct timeval tv;
    struct inetd_sess *ses;

    tv.tv_sec = INETD_WAIT_SEC;
    tv.tv_usec = 0;
    if (inetd_sock_num == 0) {
        return 0;
    }
    readable = inetd_sock_all;
    if ((n = select(inetd_sock_max + 1, &readable, (fd_set *)0, (fd_set *)0, &tv)) < 0) {
        if (n == SOCKET_ERROR && !(ISSOCKETERROR(WSAEINTR))) {
            inetd_log_message(INETD_LOG_ERROR, "inetd_serv_step: select - %m");
            return 0;
        }
    }
    else if (n > 0) {
        for (sep = inetd_serv_tab; n && sep; sep = sep->is_next) {
            if (sep->is_fd != INVALID_SOCKET && FD_ISSET(sep->is_fd, &readable)) {
                n--;
                if (!sep->is_wait && sep->is_socktype == SOCK_STREAM) {
                    ctrl = accept(sep->is_fd, (struct sockaddr *)0, (int *)0);
                    if (ctrl < 0) {
                        if (!(ISSOCKETERROR(WSAEINTR)))
                            inetd_log_message(INETD_LOG_WARN, "inetd_serv_step: accept(%s) - %m", sep->is_service);
                        return 0;
                    }
                    /*
                     * Call tcpmux to find the real service to exec.
                     */
                    if (sep->is_builtin && sep->is_builtin->ib_func == (void (*)())tcpmux) {
                        sep = tcpmux(ctrl);
                        if (sep == NULL) {
                            inetd_net_close(&ctrl);
                            return 1;
                        }
                    }
                } else
                    ctrl = sep->is_fd;
                ses = 0;
                if (sep->is_count++ == 0)
                    (void)gettimeofday(&sep->is_time, (struct timezone *)0);
                else if (sep->is_count >= inetd_serv_toomany) {
                    struct timeval now;
                    
                    (void)gettimeofday(&now, (struct timezone *)0);
                    if (now.tv_sec - sep->is_time.tv_sec >
                        INETD_CNT_INTVL) {
                        sep->is_time = now;
                        sep->is_count = 1;
                    } else {
                        inetd_log_message(INETD_LOG_ERROR,
                                          "%s/%s server failing (looping), service terminated",
                                          sep->is_service, sep->is_proto);
                        inetd_serv_close(sep);
                        inetd_net_close(&ctrl);
                        return 1;
                    }
                }
                ses = inetd_sess_open(ctrl, sep, (int *)&sep->is_wait);
                if (!ses) { /* Failed */
                    inetd_log_message(INETD_LOG_WARN, "Service %s failed.", sep->is_server);
                    if (!sep->is_wait &&
                        sep->is_socktype == SOCK_STREAM)
                        inetd_net_close(&ctrl);
                    sleep(1000);
                    inetd_serv_setup(sep);  /* if not is_up */
                } else if (sep->is_wait) {
                    sep->is_wait = (long)ses;   /* to be questioned */
                    if (sep->is_fd >= 0) {
                        FD_CLR(sep->is_fd, &inetd_sock_all);
                        inetd_sock_num--;
                        sep->is_up = 0;
                    }
                }
            }
        }
    }
    return 1;
}

/* Stop inetd service
 */
void inetd_serv_stop() {
    struct inetd_serv *sep;
    inetd_sess_stop();   /* stop sessions */
    for (sep = inetd_serv_tab; sep; sep = sep->is_next)
        inetd_serv_close(sep);
}

/* Reap inetd service
 * Accepts: * pointer to service
 */
/*
void inetd_serv_reap(struct inetd_serv *sep) {
    if (!sep->is_up) {
        inetd_log_message(INETD_LOG_INFO, "Restored %s, fd %d.",
                          sep->is_service, sep->is_fd);
        FD_SET(sep->is_fd, &inetd_sock_all);
        inetd_sock_num++;
        sep->is_up = 1;
        if (sep->is_wait)
            sep->is_wait = 1;
    }
}
*/
