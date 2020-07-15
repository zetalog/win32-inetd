/*
 * Copyright (c) 2001
 *	Prospect Information & Instrument Co. Ltd.  All rights reserved.
 *      Author: Lv "Zetalog" Zheng
 *      Internet: zl@prospect.com.cn
 *
 * Redistribution and use in source and binary forms in company, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Prospect Information
 *	& Instrument Co, Ltd.
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
 *	@(#)redirect.c: standard handles redirection routines
 * $Id: redirect.c,v 1.2 2002/01/21 11:20:09 zlv Exp $
 */

#include <windows.h>
#include <stdio.h>
#include "redirect.h"

BOOL WINAPI IsWinNT() {
    /* get windows version */
    DWORD WindowsVersion = GetVersion();
    DWORD WindowsMajorVersion = (DWORD)(LOBYTE(LOWORD(WindowsVersion)));
    DWORD WindowsMinorVersion = (DWORD)(HIBYTE(LOWORD(WindowsVersion)));

    /* Running on WIN9x? */
    if (WindowsVersion >= 0x80000000)
        return FALSE;

    /* Running on NT */
    return TRUE;
}

/* internals */
int redirect_stdout_thread(REDIRECT *redirect, HANDLE stdout_read);
int redirect_stderr_thread(REDIRECT *redirect, HANDLE stderr_read);
int redirect_process_thread(REDIRECT *redirect);
HANDLE prep_launch_redirected_child(char *cmdline, HANDLE handle_stdout,
                                    HANDLE handle_stdin, HANDLE handle_stderr,
                                    int show_child);

static int static_stdout_thread(REDIRECT *redirect) {
    return redirect_stdout_thread(redirect, redirect->stdout_read);
}

static int static_stderr_thread(REDIRECT *redirect) {
    return redirect_stderr_thread(redirect, redirect->stderr_read);
}

static int static_process_thread(REDIRECT *redirect) {
    return redirect_process_thread(redirect);
}

/* Create standards' redirector
 * Returns: * pointer to redirector
 */
REDIRECT *redirect_create() {
    REDIRECT *red = malloc(sizeof (REDIRECT));
    if (red) {
        red->child_stdin = NULL;
        red->child_stdout = NULL;
        red->child_stderr = NULL;
        red->exit_event = NULL;
        red->stdin_write = NULL;
        red->stdout_read = NULL;
        red->stderr_read = NULL;
        red->child_process = NULL;
        red->stdout_thread = NULL;
        red->stderr_thread = NULL;
        red->process_thread = NULL;
	    red->run_thread = FALSE;
    }
    return red;
}

/* Destroy standards' redirector
 * Accepts: ** pointer to redirector
 */
void redirect_destroy(REDIRECT **redirect) {
    if (!redirect || !*redirect)
        return;
    redirect_terminate_child(*redirect);
    free(*redirect);
    *redirect = NULL;
}

/* Try to start child
 * Accepts: * pointer to redirector
 */
void redirect_terminate_child(REDIRECT *redirect) {
    redirect->run_thread = FALSE;
    SetEvent(redirect->exit_event);
    Sleep(500);

    /* Check the process thread. */
    if (redirect->process_thread) {
        WaitForSingleObject(redirect->process_thread, 1000);
        redirect->process_thread = NULL;
    }

    /* Close all child handles first. */
	if (redirect->child_stdin != NULL)
        CloseHandle(redirect->child_stdin);
    redirect->child_stdin = NULL;
    if (redirect->child_stdout != NULL)
        CloseHandle(redirect->child_stdout);
    redirect->child_stdout = NULL;
    if (redirect->child_stderr != NULL)
        CloseHandle(redirect->child_stderr);
    redirect->child_stderr = NULL;
    Sleep(100);

    /* Close all parent handles. */
    if (redirect->stdin_write != NULL)
        CloseHandle(redirect->stdin_write);
    redirect->stdin_write = NULL;
    if (redirect->stdout_read != NULL)
        CloseHandle(redirect->stdout_read);
    redirect->stdout_read = NULL;
    if (redirect->stderr_read != NULL)
        CloseHandle(redirect->stderr_read);
    redirect->stderr_read = NULL;
    Sleep(100);

    /* Stop the stdout read thread. */
    if (redirect->stdout_thread != NULL) {
        if (!IsWinNT())
            TerminateThread(redirect->stdout_thread, 1);
        WaitForSingleObject(redirect->stdout_thread, 1000);
        redirect->stdout_thread = NULL;
    }

    /* Stop the stderr read thread. */
    if (redirect->stderr_thread != NULL) {
        if (!IsWinNT())
            TerminateThread(redirect->stderr_thread, 1);
        WaitForSingleObject(redirect->stderr_thread, 1000);
        redirect->stderr_thread = NULL;
    }
    Sleep(100);

    /* Stop the child process if not already stopped.
     * It's not the best solution, but it is a solution.
     * On Win98 it may crash the system if the child process is the COMMAND.COM.
     * The best way is to terminate the COMMAND.COM process with an "exit" command.
     */

    if (redirect_is_child_running(redirect)) {
        TerminateProcess(redirect->child_process, 1);
        WaitForSingleObject(redirect->child_process, 1000);
    }
    redirect->child_process = NULL;

    /* cleanup the exit event */
    if (redirect->exit_event != NULL)
        CloseHandle(redirect->exit_event);
    redirect->exit_event = NULL;
}

/* Check if the child process is running
 * On NT/2000 the handle must have PROCESS_QUERY_INFORMATION access.
 * Accepts: * pointer to redirector
 * Returns: TRUE if running while FALSE not
 */
int redirect_is_child_running(REDIRECT *redirect) {
    DWORD dwExitCode;
    if (redirect->child_process == NULL)
        return FALSE;
    GetExitCodeProcess(redirect->child_process, &dwExitCode);
    return (dwExitCode == STILL_ACTIVE) ? TRUE: FALSE;
}

/* Create standard handles, try to start child from command line
 * Accepts: * pointer to redirector
 *          * pointer to command line string]
 *          whether show child window
 * Returns: TRUE if succeeded while FALSE not
 */
int redirect_start_child(REDIRECT *redirect, char *cmdline, int show) {
    HANDLE hProcess = GetCurrentProcess();
    HANDLE hStdInWriteTmp, hStdOutReadTmp, hStdErrReadTmp;
    DWORD dwThreadID;

    /* Set up the security attributes struct. */
    SECURITY_ATTRIBUTES sa;
    ZeroMemory(&sa, sizeof (SECURITY_ATTRIBUTES));
    sa.nLength = sizeof (SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    /* Create the child stdin pipe. */
    CreatePipe(&redirect->child_stdin, &hStdInWriteTmp, &sa, 0);

    /* Create the child stdout pipe. */
    CreatePipe(&hStdOutReadTmp, &redirect->child_stdout, &sa, 0);

    /* Create the child stderr pipe. */
    CreatePipe(&hStdErrReadTmp, &redirect->child_stderr, &sa, 0);

    /* Create new stdin write, stdout and stderr read handles.
     * Set the properties to FALSE. Otherwise, the child inherits the
     * properties and, as a result, non-closeable handles to the pipes
     * are created.
     */
    DuplicateHandle(hProcess, hStdInWriteTmp,
                    hProcess, &redirect->stdin_write,
                    0, FALSE, DUPLICATE_SAME_ACCESS);

    DuplicateHandle(hProcess, hStdOutReadTmp,
                    hProcess, &redirect->stdout_read,
                    0, FALSE, DUPLICATE_SAME_ACCESS);

    DuplicateHandle(hProcess, hStdErrReadTmp,
                    hProcess, &redirect->stderr_read,
                    0, FALSE, DUPLICATE_SAME_ACCESS);

    /* Close inheritable copies of the handles you do not want to be
     * inherited.
     */
    CloseHandle(hStdInWriteTmp);
    CloseHandle(hStdOutReadTmp);
    CloseHandle(hStdErrReadTmp);

    /* Start child process with redirected stdout, stdin & stderr */
    redirect->child_process = prep_launch_redirected_child(cmdline,
                                                           redirect->child_stdout,
                                                           redirect->child_stdin,
                                                           redirect->child_stderr,
                                                           show);

    if (redirect->child_process == NULL) {
        TCHAR buffer[BUFFER_SIZE];
        sprintf(buffer, "Unable to start %s\n", cmdline);
        redirect_child_stdout_write(redirect, buffer);

        /* close all handles and return FALSE */
        CloseHandle(redirect->child_stdin);
        redirect->child_stdin = NULL;
        CloseHandle(redirect->child_stdout);
        redirect->child_stdout = NULL;
        CloseHandle(redirect->child_stderr);
        redirect->child_stderr = NULL;

        return FALSE;
    }

    redirect->run_thread = TRUE;

    /* Create Exit event */
    redirect->exit_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!redirect->exit_event)
        goto cleanup;

    /* Launch the thread that read the child stdout. */
    redirect->stdout_thread = CreateThread(NULL, 0, 
                                           (LPTHREAD_START_ROUTINE)static_stdout_thread,
                                           (LPVOID)redirect, 0, &dwThreadID);
    if (!redirect->stdout_thread)
        goto cleanup;

    /* Launch the thread that read the child stderr. */
    redirect->stderr_thread = CreateThread(NULL, 0,
                                           (LPTHREAD_START_ROUTINE)static_stderr_thread,
                                           (LPVOID)redirect, 0, &dwThreadID);
    if (!redirect->stderr_thread)
        goto cleanup;

    /* Launch the thread that monitoring the child process. */
    redirect->process_thread = CreateThread(NULL, 0,
                                            (LPTHREAD_START_ROUTINE)static_process_thread,
                                            (LPVOID)redirect, 0, &dwThreadID);
    if (!redirect->process_thread)
        goto cleanup;

    /* Virtual function to notify derived class that the child is started. */
    redirect_child_started(redirect, cmdline);

    return TRUE;
cleanup:
    if (redirect->exit_event) {
        CloseHandle(redirect->exit_event);
        redirect->exit_event = NULL;
    }
    if (redirect->stdout_thread) {
        if (!IsWinNT())
            TerminateThread(redirect->stdout_thread, 1);
        WaitForSingleObject(redirect->stdout_thread, 1000);
        redirect->stdout_thread = NULL;
    }
    if (redirect->stderr_thread) {
        if (!IsWinNT())
            TerminateThread(redirect->stderr_thread, 1);
        WaitForSingleObject(redirect->stderr_thread, 1000);
        redirect->stderr_thread = NULL;
    }
    if (redirect->process_thread) {
        TerminateProcess(redirect->child_process, 1);
        WaitForSingleObject(redirect->child_process, 1000);
        redirect->process_thread = NULL;
    }
    return FALSE;
}

/* Thread to read the child stdout
 * Accepts: * pointer to redirector
 *          stdout read handle
 * Returns: thread exit value
 */
int redirect_stdout_thread(REDIRECT *redirect, HANDLE stdout_read) {
    DWORD bytes;
    CHAR buffer[BUFFER_SIZE+1];
    
    while (redirect->run_thread) {
        if (!ReadFile(stdout_read, buffer, BUFFER_SIZE, &bytes, NULL) || !bytes) {
            if (GetLastError() == ERROR_BROKEN_PIPE)
                break;          /* pipe done - normal exit path. */
            else
                return GetLastError();   /* Something bad happened. */
        }
        if (bytes) {
            /* Virtual function to notify derived class that
             * characters are writted to stdout.
             */
            buffer[bytes] = '\0';
            redirect_child_stdout_write(redirect, buffer);
        }
    }
    return 0;
}

/* Thread to read the child stderr
 * Accepts: * pointer to redirector
 *          stderr read handle
 * Returns: thread exit value
 */
int redirect_stderr_thread(REDIRECT *redirect, HANDLE stderr_read) {
    DWORD bytes;
    CHAR buffer[BUFFER_SIZE+1];

    while (redirect->run_thread) {
        if (!ReadFile(stderr_read, buffer, BUFFER_SIZE, &bytes, NULL) || !bytes) {
            if (GetLastError() == ERROR_BROKEN_PIPE)
                break;          /* pipe done - normal exit path. */
            else
                return GetLastError();   /* Something bad happened. */
        }
        if (bytes) {
            /* Virtual function to notify derived class that
             * characters are writted to stderr.
             */
            buffer[bytes] = '\0';
            redirect_child_stderr_write(redirect, buffer);
        }
    }
    return 0;
}

/* Thread to monitoring the child process
 * Accepts: * pointer to redirector
 * Returns: process exit value
 */
int redirect_process_thread(REDIRECT *redirect) {
    HANDLE wait_handles[2];
    wait_handles[0] = redirect->exit_event;
    wait_handles[1] = redirect->child_process;

    while (redirect->run_thread) {
        switch (WaitForMultipleObjects(2, wait_handles, FALSE, 1)) {
        case WAIT_OBJECT_0 + 0: /* exit on event */
            break;        
        case WAIT_OBJECT_0 + 1: /* child process exit */
            redirect->run_thread = FALSE;
            break;
        }
    }
    /* Virtual function to notify derived class that
     * child process is terminated.
     * Application must call TerminateChildProcess()
     * but not direcly from this thread!
     */
    redirect_child_terminated(redirect);
    return GetLastError();
}

/* Write input to the child stdin
 * Accepts: * pointer to redirector
 *          * pointer to input string
 */
void redirect_write_child_stdin(REDIRECT *redirect, char *input) {
    DWORD bytes;
    DWORD length = strlen(input);
    if (redirect->stdin_write != NULL && length > 0) {
        if (!WriteFile(redirect->stdin_write, input, length, &bytes, NULL)) {
            if (GetLastError() == ERROR_NO_DATA)
                ;       /* Pipe was closed (do nothing). */
            else
                return; /* Something bad happened. */
        }
    }
}

/* Launch the process that you want to redirect
 * Accepts: * pointer to command line string
 *          stdout handle
 *          stdin handle
 *          stderr handle
 *          whether show child window
 * Returns: child process handle
 */
HANDLE prep_launch_redirected_child(char *cmdline, HANDLE handle_stdout,
                                    HANDLE handle_stdin, HANDLE handle_stderr,
                                    int show_child) {
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    HANDLE hProcess = GetCurrentProcess();
    /* Create the NULL security token for the process */
    LPVOID lpSD = NULL;
    LPSECURITY_ATTRIBUTES lpSA = NULL;
    BOOL result;

    /* Set up the start up info struct. */
    ZeroMemory(&si, sizeof (STARTUPINFO));
    si.cb = sizeof (STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = handle_stdout;
    si.hStdInput  = handle_stdin;
    si.hStdError  = handle_stderr;

    /* Use this if you want to show the child.
     * Note that dwFlags must include STARTF_USESHOWWINDOW if you want to
     * use the wShowWindow flags.
     */    
    si.wShowWindow = show_child ? SW_SHOW: SW_HIDE;

    /* On NT/2000 the handle must have PROCESS_QUERY_INFORMATION access.
     * This is made using an empty security descriptor. It is not the same
     * as using a NULL pointer for the security attribute!
     */    
    if (IsWinNT()) {
        lpSD = GlobalAlloc(GPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
        InitializeSecurityDescriptor(lpSD, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(lpSD, -1, 0, 0);

        lpSA = (LPSECURITY_ATTRIBUTES)GlobalAlloc(GPTR, sizeof (SECURITY_ATTRIBUTES));
        lpSA->nLength = sizeof (SECURITY_ATTRIBUTES);
        lpSA->lpSecurityDescriptor = lpSD;
        lpSA->bInheritHandle = TRUE;
    }

    /* Try to spawn the process. */
    result = CreateProcess(NULL, (char*)cmdline, lpSA, NULL, TRUE,
                           CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);

    /* Cleanup memory allocation */
    if (lpSA != NULL)
        GlobalFree(lpSA);
    if (lpSD != NULL)
        GlobalFree(lpSD);

    /* Return if an error occurs. */
    if (!result)
        return FALSE;

    /* Close any unnecessary handles. */
    CloseHandle(pi.hThread);

    /* Save global child process handle to cause threads to exit. */
    return pi.hProcess;
}
