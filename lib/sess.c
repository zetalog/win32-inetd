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
 * @(#)session.c: inetd network session routines
 * $Id: sess.c,v 1.7 2002/02/05 05:16:09 zlv Exp $
 */

#include <inetd.h>
#include <inetd_int.h>
#include "misc.h"

/* externals */
int inetd_sess_end = 0;

/* threads */
void input_thread(LPVOID data);
void output_thread(LPVOID data);
void session_thread(PVOID data);

/* Session thread
 * Accepts: not fully qulified session pointer
 */
void session_thread(PVOID data) {
    struct inetd_sess *ses;
    SECURITY_ATTRIBUTES sa;
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    DWORD id;
    HANDLE hdls[3];
    HANDLE ipipe = NULL;
    HANDLE opipe = NULL;
    /*struct hostent *hn;*/
    struct sockaddr_in from;
    int fromlen = sizeof (from);
    int term = 0;
    struct inetd_serv *sep;
    void *penv = NULL;

    ses = (struct inetd_sess *)data;
    if (!ses || !(sep = ses->is_serv)) {
        return;    /* Couldn't get memory for ses */
    }

    if (sep->is_builtin)
        (*sep->is_builtin->ib_func)(ses->is_socket, sep);
    else {
        /* Create redirected pipe */
        sa.nLength = sizeof (sa);   /* Use default ACL */
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;   /* Inherit handles */
        if (!CreatePipe(&ses->is_ihandle, &ipipe, &sa, 0))
            printf("input pipe create failure %d.", GetLastError());
        else if (!CreatePipe(&opipe, &ses->is_ohandle, &sa, 0))
            printf("output pipe create failure %d.", GetLastError());
        else {  /* Instantiate daemon */
            char *remote_addr = NULL;
            unsigned short remote_port = 0;
            char *env_buf = NULL;
            si.cb = sizeof (STARTUPINFO);
            si.lpReserved = NULL;
            si.lpTitle = NULL;
            si.lpDesktop = NULL;
            si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0;
            si.wShowWindow = SW_SHOW;
            si.lpReserved2 = NULL;
            si.cbReserved2 = 0;
            si.dwFlags = STARTF_USESTDHANDLES;
            si.hStdInput  = opipe;
            si.hStdOutput = ipipe;
            /* Errors go to same place as output */
            DuplicateHandle(GetCurrentProcess(), ipipe,
                            GetCurrentProcess (), &si.hStdError,
                            DUPLICATE_SAME_ACCESS, TRUE, 0);
            EnterCriticalSection(&inetd_mutex_env);
            if (getpeername(ses->is_socket, (struct sockaddr *)&from, &fromlen)) {
                remote_addr = "unknown";
                inetd_log_message(INETD_LOG_WARN, "Service %s (%s): UNKNOWN client.",
                                  ses->is_name,
                                  ses->is_program);
            } else {
                remote_addr = inet_ntoa(from.sin_addr);
                /*hn = gethostbyaddr((char *)&from.sin_addr,
                                   sizeof (struct in_addr),
                                   from.sin_family);
                remote_host = hn ? hn->h_name : remote_addr;*/
                remote_port = ntohs(from.sin_port);
                inetd_log_message(INETD_LOG_INFO, "Service %s (%s)£º %s:%d",
                                  ses->is_name,
                                  ses->is_program,
                                  remote_addr,
                                  remote_port);
            }
            env_buf = malloc(strlen(remote_addr) + 20);
            if (env_buf) {
                sprintf(env_buf, "REMOTE_ADDR=%s", remote_addr);
                putenv(env_buf);
                sprintf(env_buf, "REMOTE_PORT=%d", remote_port);
                putenv(env_buf);
                free(env_buf);
            }
            penv = GetEnvironmentStrings();
            if (!CreateProcess(ses->is_program, ses->is_command, NULL, NULL, TRUE, DETACHED_PROCESS, penv, NULL, &si, &pi))
                printf("process create failure %d.", GetLastError());
            else {  /* Got daemon, note its process */
                ses->is_daemon = pi.hProcess;
                CloseHandle(pi.hThread);
            }
            if (penv)
                FreeEnvironmentStrings(penv);
            LeaveCriticalSection(&inetd_mutex_env);
            CloseHandle(ipipe);     /* Flush our copy of pipes */
            CloseHandle(opipe);
            ipipe = opipe = NULL;
            if (ses->is_daemon) {   /* Only if started daemon */
                sa.bInheritHandle = FALSE;	/* No inheritance */
                /* Create threads */
                if (!(ses->is_ithread =	CreateThread(&sa, 0, (LPTHREAD_START_ROUTINE)input_thread, (LPVOID)ses, 0, &id))) {
                    printf("input thread create failure %d.", GetLastError());
                    inetd_net_close(&ses->is_socket);
                }
                else if (!(ses->is_othread = CreateThread(&sa, 0, (LPTHREAD_START_ROUTINE)output_thread, (LPVOID)ses, 0, &id))) {
                    printf("output pipe create failure %d.", GetLastError());
                    inetd_net_close(&ses->is_socket);
                    TerminateThread(ses->is_ithread,0);
                }
                else {  /* wait for completion */
                    hdls[0] = ses->is_ithread;
                    hdls[1] = ses->is_othread;
                    hdls[2] = ses->is_daemon;
                    while (!term) {
                        if (inetd_sess_end) {
                            TerminateThread(ses->is_ithread, 0);
                            TerminateThread(ses->is_othread, 0);
                            TerminateProcess(ses->is_daemon, -1);
                            break;
                        }
                        switch (WaitForMultipleObjects(3, hdls, 0, 200)-WAIT_OBJECT_0) {
                        case 0:
                            TerminateThread(ses->is_othread, 0);
                            TerminateProcess(ses->is_daemon, 1);
                            term = 1;
                            break;
                        case 1:
                            TerminateThread(ses->is_ithread, 0);
                            TerminateProcess(ses->is_daemon, 1);
                            term = 1;
                            break;
                        case 2:
                            TerminateThread(ses->is_othread,0);
                            TerminateThread(ses->is_ithread,0);
                            term = 1;
                            break;
                        case WAIT_TIMEOUT:
                            continue;
                        default:
                            printf("error %d in WaitForMultipleObjects().", GetLastError());
                            TerminateThread(ses->is_ithread, 0);
                            TerminateThread(ses->is_othread, 0);
                            TerminateProcess(ses->is_daemon, -1);
                            term = 1;
                            break;
                        }
                    }
                }
            }
        }
        /* Clean up */
        if (ipipe)
            CloseHandle(ipipe);
        if (opipe)
            CloseHandle(opipe);
    }
    if (!inetd_sess_end)
        inetd_serv_setup(ses->is_serv);
    inetd_sess_close(ses);
    ExitThread(0);
}

/* Input thread
 * Accepts: session pointer
 */
void input_thread(LPVOID data) {
    struct inetd_sess *ses = (struct inetd_sess *)data;
    char buf[INETD_BUF_SIZE];
    DWORD i;
    while (ReadFile(ses->is_ihandle, buf, INETD_BUF_SIZE, &i, NULL) && (send(ses->is_socket, buf, i, 0) > 0));
    switch (GetLastError()) {
    case ERROR_BROKEN_PIPE:
    case WSAECONNABORTED:
        break;
    default:
        printf("input thread error %d.", GetLastError());
    }
    closesocket(ses->is_socket);
}


/* Output thread
 * Accepts: session pointer
 */
void output_thread(LPVOID data) {
    struct inetd_sess *ses = (struct inetd_sess *)data;
    char buf[1];
    DWORD i;
    while ((i = recv(ses->is_socket, buf, 1, 0) > 0) && WriteFile(ses->is_ohandle, buf, 1, &i, NULL));
    closesocket(ses->is_socket);
}

/* Open a network session
 * Accepts: socket handle
 *          * pointer to program path string
 *          * pointer to command line string
 * Returns: * pointer to session structure
 */
struct inetd_sess *inetd_sess_open(SOCKET fd, struct inetd_serv *serv, int *tid) {
    struct inetd_sess *ses;
    SECURITY_ATTRIBUTES sa;
    HANDLE hdl;
    ses = (struct inetd_sess *)memset(fsget(sizeof (struct inetd_sess)),
                                      0, sizeof (struct inetd_sess));
    ses->is_serv = serv;
    ses->is_socket = fd;
    sa.nLength = sizeof (sa);	/* use default ACL w/ no inheritance */
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = FALSE;
    if (!(hdl = CreateThread(&sa, 0, (LPTHREAD_START_ROUTINE)session_thread, (LPVOID)ses, 0, 0)))
        inetd_log_message(INETD_LOG_FAILURE, "inetd_sess_open: CreateThread - %m");
    CloseHandle(hdl);
    return ses;
}

/* Close a network session
 * Accepts: ** pointer to session structure
 */
void inetd_sess_close(struct inetd_sess *ses) {
    struct linger linger = { 1,0 };
    if (ses->is_socket != INVALID_SOCKET) {
        setsockopt(ses->is_socket, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(linger));
        shutdown(ses->is_socket, SD_BOTH);
        inetd_net_close(&ses->is_socket);
    }
    if (ses->is_daemon)
        CloseHandle(ses->is_daemon);
    if (ses->is_ithread)
        CloseHandle(ses->is_ithread);
    if (ses->is_othread)
        CloseHandle(ses->is_othread);
    if (ses->is_ihandle)
        CloseHandle(ses->is_ihandle);
    if (ses->is_ohandle)
        CloseHandle(ses->is_ohandle);
    fsgive((void **)&ses);
}

/* Stop network sessions
 */
void inetd_sess_stop() {
    inetd_sess_end = 1;
}
