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
 * @(#)error.c: inetd configuration error informings
 * $Id: error.c,v 1.2 2002/01/23 13:04:23 zlv Exp $
 */

#include <inetd.h>
#include "inetd_cpl.h"

void inetd_error_display() {
    int ecode;
    int ret;
    char *message = NULL;

    ecode = GetLastError();
    ret = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        NULL,
                        GetLastError(),
                        LANG_NEUTRAL,
                        (LPTSTR)&message,
                        0,
                        NULL);
    message[lstrlen(message)-2] = TEXT('\0');  /* remove cr and newline character */
    MessageBox(NULL, message, NULL, MB_ICONSTOP);
    if (message)
        LocalFree((HLOCAL)message);
}

void inetd_error_prompt(char *format, ...) {
    va_list pvar;
    char message[INETD_BUF_SIZE];

    va_start(pvar, format);
    *message = '\0';
    if (_vsnprintf(message, INETD_BUF_SIZE-1, format, pvar) < 0) {
        message[INETD_BUF_SIZE-1] = '\0';
    }
    MessageBox(NULL, message, NULL, MB_ICONSTOP);
    va_end(pvar);
    return;
}
