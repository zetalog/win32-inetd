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
 * @(#)applet.c: inetd control panel applet entrance
 * $Id: applet.c,v 1.2 2002/01/16 13:11:38 zlv Exp $
 */

#include <inetd.h>
#include "inetd_cpl.h"

LONG CALLBACK CPlApplet(HWND hcpl, UINT msg, LPARAM lparam1, LPARAM lparam2) { 
    int i;
    LPCPLINFO cpl_info;

    i = (int)lparam1;

    switch (msg) {
    case CPL_INIT:      /* first message, sent once */
        hinst = GetModuleHandle("inetd.cpl");
        return (hinst ? TRUE : FALSE);
    case CPL_GETCOUNT:  /* second message, sent once */
        return NUM_SUBPROGS;
        break;
    case CPL_INQUIRE:   /* third message, sent once per application */
        cpl_info = (LPCPLINFO)lparam2;
        cpl_info->lData = 0;
        cpl_info->idIcon = inetd_cpl_prog[i].cp_icon;
        cpl_info->idName = inetd_cpl_prog[i].cp_name;
        cpl_info->idInfo = inetd_cpl_prog[i].cp_desc;
        break;
    case CPL_DBLCLK:    /* application icon double-clicked */
        (*inetd_cpl_prog[i].cp_func)(NULL,
                                     inetd_cpl_prog[i].cp_icon,
                                     inetd_cpl_prog[i].cp_title,
                                     inetd_cpl_prog[i].cp_sheets);
        break;
    case CPL_STOP:      /* sent once per application before CPL_EXIT */
        break;
    case CPL_EXIT:      /* sent once before FreeLibrary is called */
        break;
    default:
        break;
    }
    return 0; 
} 
