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
 * @(#)ntserv.h: windows nt service interfaces
 * $Id: ntserv.h,v 1.2 2002/01/21 11:20:04 zlv Exp $
 */

#ifndef __NT_SERVICE_H
#define __NT_SERVICE_H

/* externals */
extern HANDLE stopevent;
extern BOOL debug;

/* functions */
extern int ntserv_install();
extern int ntserv_remove();
extern int ntserv_debug(int argc, char ** argv, int faceless);
extern int ntserv_start(DWORD dwArgc, LPTSTR *lpszArgv);
extern int ntserv_end();
extern int ntserv_report(DWORD current, DWORD ecode, DWORD waithint);

/* overloads */
void ntserv_create();
int ntserv_init();
int ntserv_work();
void ntserv_fin();
void ntserv_destroy();
void ntserv_pause();
void ntserv_continue();
void ntserv_shutdown();

#endif /* __NT_SERVICE_H */
