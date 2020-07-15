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
 * @(#)main.c: inetd config application main
 * $Id: main.c,v 1.3 2002/01/23 13:04:27 zlv Exp $
 */

#include <inetd.h>
#include "inetd_cpl.h"

#define MAINWNDCLASS    "InetdConfigerWndClass"
#define WINDOWNAME      "InetdConfiger"
#define MUTEXNAME       "InetdConfigerAlreadyRunning"
#define INETD_CPL_MAIN  0

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    HANDLE	hMutex;
    
    hMutex = CreateMutex(NULL, FALSE, MUTEXNAME);
#ifndef _DEBUG
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        ReleaseMutex(hMutex);
        return INETD_FALSE;
    }
#endif
    hinst = hInstance;
    
    (*inetd_cpl_prog[INETD_CPL_MAIN].cp_func)(NULL,
                                              inetd_cpl_prog[INETD_CPL_MAIN].cp_icon,
                                              inetd_cpl_prog[INETD_CPL_MAIN].cp_title,
                                              inetd_cpl_prog[INETD_CPL_MAIN].cp_sheets);
    ReleaseMutex(hMutex);
    return INETD_TRUE;
}
