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
 * @(#)inetd.h: internet daemon service generic header
 * $Id: inetd.h,v 1.13 2002/02/20 07:53:45 zlv Exp $
 */

#ifndef __INETD_H
#define __INETD_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <winsock2.h>
#include <ctype.h>
#include <process.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>
#include <tchar.h>

/* definations */
#define INETD_FALSE     0
#define INETD_TRUE      !(INETD_FALSE)

#define APP_NAME        "inetd"
#define SERVICE_NAME    "inetd"
#define DISPLAY_NAME    "Internet Super Server"
#define DEPENDENCIES    ""
#define DESCRIPTION     "Internet daemon integrates TCP and UDP servers which use standard handles serving for the clients."

#define INETD_BUF_SIZE      4096
#define INETD_LINE_MAX      2048
#define	INETD_TOOMANY       2       /* don't start more than TOOMANY */
#define	INETD_CNT_INTVL     10      /* servers in CNT_INTVL sec. */
#define INETD_WAIT_SEC      2       /* select waits */

#define INETD_NORM_TYPE     0
#define INETD_MUX_TYPE      1
#define INETD_MUXPLUS_TYPE  2

#define INETD_ISMUX(sep)    (((sep)->is_type == INETD_MUX_TYPE) || \
	                         ((sep)->is_type == INETD_MUXPLUS_TYPE))
#define INETD_ISMUXP(sep)   ((sep)->is_type == INETD_MUXPLUS_TYPE)
#define INETD_SERV_LEN  (256+2) /* 2 bytes for \r\n */

#define INETD_LOG_ERROR     EVENTLOG_ERROR_TYPE
#define INETD_LOG_WARN      EVENTLOG_WARNING_TYPE
#define INETD_LOG_INFO      EVENTLOG_INFORMATION_TYPE
#define INETD_LOG_SUCCESS   EVENTLOG_AUDIT_SUCCESS
#define INETD_LOG_FAILURE   EVENTLOG_AUDIT_FAILURE
#define INETD_LOG_DEBUG     (int)-1

#define INETD_SET_SERVICE   1
#define INETD_SET_USER      2
#define INETD_SET_SERVER    3
#define INETD_SET_COMMAND   4
#define INETD_SET_PROTO     5
#define INETD_SET_WAIT      6
#define INETD_SET_SOCKTYPE  7
#define INETD_SET_TYPE      8

#define INETD_CONF_LOAD     1
#define INETD_CONF_SAVE     2

#define INETD_REPORT_INFO   0
#define INETD_REPORT_ERROR  1

/* structures */
struct inetd_serv {
    char               *is_service;     /* name of service */
    int                 is_socktype;    /* type of socket to use */
    char               *is_proto;       /* protocol used */
    long                is_wait;        /* single threaded server */
    short               is_checked;     /* looked at during merge */
    short               is_up;          /* set to the serving set */
    char               *is_user;        /* user name to run as */
    char               *is_server;      /* server program */
    char               *is_cmdline;     /* program commond line */
    SOCKET              is_fd;          /* open descriptor */
    int                 is_type;        /* type */
    struct sockaddr_in  is_ctrladdr;    /* bound address */
    int                 is_count;       /* number started since is_time */
    struct timeval      is_time;        /* start of se_count */
	struct inetd_builtin *is_builtin;   /* if built-in, description */
    struct inetd_serv  *is_next;        /* next service */
};

struct inetd_sess {
    struct inetd_serv  *is_serv;        /* service */
#define is_name         is_serv->is_service
#define is_program      is_serv->is_server
#define is_command      is_serv->is_cmdline
    HANDLE              is_daemon;      /* daemon's thread */
    HANDLE              is_ithread;     /* input thread */
    HANDLE              is_othread;     /* output thread */
    HANDLE              is_ihandle;     /* input handle */
    HANDLE              is_ohandle;     /* output handle */
    SOCKET              is_socket;      /* network handle */
};

/* externals */
struct inetd_serv *inetd_serv_tab;

/* functions */
void inetd_log_message(int level, const char *format, ...);
char *inetd_log_sid();

int inetd_conf_open(int how);
void inetd_conf_close();
struct inetd_serv *inetd_conf_load();
int inetd_conf_save(struct inetd_serv *cp);
struct inetd_serv *inetd_conf_alloc(struct inetd_serv *cp);
void inetd_conf_free(struct inetd_serv *cp);
void inetd_conf_set(struct inetd_serv *cp, int what, void *value);
void inetd_conf_copy(struct inetd_serv *cp, struct inetd_serv *value);
int inetd_conf_compare(struct inetd_serv *cp, struct inetd_serv *value);

int inetd_serv_start();
int inetd_serv_step();
void inetd_serv_stop();
void inetd_serv_setup(struct inetd_serv *sep);

struct inetd_sess *inetd_sess_open(SOCKET fd, struct inetd_serv *serv, int *tid);
void inetd_sess_close(struct inetd_sess *ses);
void inetd_sess_stop();

/* WSAs */
void inetd_net_init();
void inetd_net_fin();
void inetd_net_close(SOCKET *fdp);
int inetd_net_getline(SOCKET fd, char *buf, int len);
int inetd_net_recv(SOCKET fd, char *buf, int len);
int inetd_net_recvfrom(SOCKET fd, char *buf, int len, struct sockaddr *from, int *fromlen);

int inetd_os_win95();

struct inetd_builtin {
    char    *ib_service;    /* internally provided service name */
    int     ib_socktype;    /* type of socket supported */
    long    ib_wait;        /* 1 if should wait for child */
    void	(*ib_func)();   /* function which performs it */
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INETD_H */
