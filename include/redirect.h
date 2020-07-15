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
 *	@(#)redirect.h: standard handles redirection interfaces
 * $Id: redirect.h,v 1.3 2002/02/20 03:21:02 zlv Exp $
 */

/* Advanced redirection implementation for INETD's next generation.
 */

#ifndef __REDIRECT_H
#define __REDIRECT_H

#ifdef __cplusplus
extern "C"
{
#endif

/* definitions */
#define BUFFER_SIZE 256

struct std_redirect {
    HANDLE  exit_event;
    HANDLE  child_stdin;    /* child input pipe */
    HANDLE  child_stdout;   /* child output pipe */
    HANDLE  child_stderr;   /* child error output pipe */
    /* do not access directly on members below */
    HANDLE  stdin_write;
    HANDLE  stdout_read;
    HANDLE  stderr_read;
    HANDLE  stdout_thread;
    HANDLE  stderr_thread;
    HANDLE  process_thread;
    HANDLE  child_process;
    int     run_thread : TRUE;
    void   *impart_data;    /* user data set for callbacks */
};

typedef struct std_redirect     REDIRECT;

/* callbacks */
extern void redirect_child_started(REDIRECT *redirect, char *cmdline);
extern void redirect_child_terminated(REDIRECT *redirect);
extern void redirect_child_stdout_write(REDIRECT *redirect, char *output);
extern void redirect_child_stderr_write(REDIRECT *redirect, char *output);

/* externals */
extern REDIRECT *redirect_create();
extern void redirect_destroy(REDIRECT **redirect);
extern int redirect_start_child(REDIRECT *redirect, char *cmdline, int show);
extern void redirect_terminate_child(REDIRECT *redirect);
extern int redirect_is_child_running(REDIRECT *redirect);
extern void redirect_write_child_stdin(REDIRECT *redirect, char *input);

#ifdef __cplusplus
}
#endif

#endif /* __REDIRECT_H */
