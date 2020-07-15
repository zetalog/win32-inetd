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
 * @(#)log.c: event log operation routines
 * $Id: log.c,v 1.4 2002/01/27 09:48:37 zlv Exp $
 */

#include <inetd.h>
#include "sockerr.h"
#include "event.h"

BYTE inetd_log_user[4096];
static BOOL inetd_log_init = INETD_FALSE;
static BOOL inetd_log_added = INETD_FALSE;

/* defines */
#define VA_START(a, b) va_start((a), (b))
#define va_alist ...
#define va_dcl

/* internals */
void inetd_log_varible(int level, const char *oformat, va_list pvar);
char *inetd_log_text();
void inetd_log_source(char *service, char *module);
static void replace_percentm(const char *inbuffer, char *outbuffer, int olen);

/* Replace "%m" with error message
 * Accepts: * pointer to input buffer
 *          * pointer to output buffer
 *          output buffer length
 */
static void replace_percentm(const char *inbuffer, char *outbuffer, int olen) {
    register const char *t2;
    register char *t1, ch;
    if (!outbuffer || !inbuffer)
        return;
    olen--;
    for (t1 = outbuffer; (ch = *inbuffer) && t1-outbuffer < olen ; ++inbuffer)
        if (inbuffer[0] == '%' && inbuffer[1] == 'm')
            for (++inbuffer, t2 = inetd_log_text(); (t2 && t1-outbuffer < olen) && (*t1 = *t2++); t1++);
        else *t1++ = ch;
    *t1 = '\0';
}

/* Local defined log format writing winNT event log service
 * Accepts: log level
 *          * pointer to original format
 *          varible list
 */
void inetd_log_varible(int level, const char *oformat, va_list pvar) va_dcl {
#define FMT_BUFLEN 2*1024 + 2*10
    TCHAR fmt_cpy[FMT_BUFLEN], format[FMT_BUFLEN];
    HANDLE  event;
    LPTSTR  strings[1];
    DWORD   err;
    *fmt_cpy = '\0';
    if (!oformat)
        return;
    /* Print the pid & maybe the thread id in format here.  Skip forward if */
    /* you don't want it later (e.g. if syslogging).                        */
    sprintf(format, "%05d:", (int)getpid());
    sprintf(format + strlen(format), "%06d:", (int)GetCurrentThreadId());
    strcat(format, " ");
    replace_percentm(oformat, format + strlen(format), sizeof(format) - strlen(format));
    if (_vsnprintf(fmt_cpy, FMT_BUFLEN-1, format, pvar) < 0) {
        fmt_cpy[FMT_BUFLEN-1] = '\0';
    }
    if (level == INETD_LOG_DEBUG) {
        fprintf(stderr, "%s\n", fmt_cpy);
        fflush(stderr);
    } else {
        err = GetLastError();
        if (!inetd_log_added) {
            char module[MAX_PATH];
            GetModuleFileName(NULL, module, MAX_PATH);
            inetd_log_source(TEXT(SERVICE_NAME), module);
            inetd_log_added = INETD_TRUE;
        }
        inetd_log_sid();
        /* Use event logging to log the error.
         */
        event = RegisterEventSource(NULL, TEXT(SERVICE_NAME));
        strings[0] = fmt_cpy;
        if (event != NULL) {
            ReportEvent(event,          /* handle of event source */
                        (WORD)level,    /* event type */
                        0,              /* event category */
                        INETD_LOG_MSG,  /* event ID */
                        inetd_log_user, /* current user's SID */
                        1,              /* strings in strings */
                        0,              /* no bytes of raw data */
                        strings,        /* array of error strings */
                        NULL);          /* no raw data */
            (void)DeregisterEventSource(event);
        }
    }
}

/* WinNT event log service format
 * Accepts: log level
 *          * pointer to original format
 *          varible list
 */
void inetd_log_message(int level, const char *format, va_alist) va_dcl {
    va_list pvar;
    va_start(pvar, format);
    inetd_log_varible(level, format, pvar);
    va_end(pvar);
}

/* Get current error text
 * Returns: error message
 */
char *inetd_log_text() {
    DWORD ret;
    static TCHAR err_msg[256];
    LPTSTR temp = NULL;
    DWORD err = GetLastError();
    if (!err) {
        err = WSAGetLastError();
        sprintf(err_msg, TEXT("%s (%d)"), sock_strerror(err), err);
        return err_msg;
    }
    ret = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        NULL, 
                        GetLastError(),
                        LANG_NEUTRAL,
                        (LPTSTR)&temp,
                        0,
                        NULL);
    /* supplied buffer is not long enough */
    if (!ret || ((long)256 < (long)ret+14))
        err_msg[0] = TEXT('\0');
    else {
        temp[lstrlen(temp)-2] = TEXT('\0');  /* remove cr and newline character */
        sprintf(err_msg, TEXT("%s (0x%x)"), temp, GetLastError());
    }
    if (temp)
        LocalFree((HLOCAL)temp);
    return err_msg;
}

void inetd_log_source(char *service, char *module) {
    HKEY hkey;
    DWORD data;
    char key_name[1024];
    
    strcpy(key_name, "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");
    strcat(key_name, service);
    
    /* Add your source name as a subkey under the Application 
     * key in the EventLog registry key.  
     */
    if (RegCreateKey(HKEY_LOCAL_MACHINE, key_name, &hkey))
        return; /* Fatal error, no key and no way of reporting the error!!! */
    
    /* Add the name to the EventMessageFile subkey.
     */
    if (RegSetValueEx(hkey,                 /* subkey handle */
                      "EventMessageFile",   /* value name */
                      0,                    /* must be zero */
                      REG_EXPAND_SZ,        /* value type */
                      (LPBYTE)module,       /* pointer to value data */
                      strlen(module) + 1))  /* length of value data */
    {
        RegCloseKey(hkey); 
        return; /* Couldn't set key */
    }
    
    /* Set the supported event types in the TypesSupported subkey.
     */
    data = INETD_LOG_ERROR | INETD_LOG_WARN | INETD_LOG_INFO;
    if (RegSetValueEx(hkey,             /* subkey handle */
                      "TypesSupported", /* value name */
                      0,                /* must be zero */
                      REG_DWORD,        /* value type */
                      (LPBYTE)&data,    /* pointer to value data */
                      sizeof (DWORD)))  /* length of value data */
    {
        RegCloseKey(hkey); 
        return; /* Couldn't set key */
    }
    RegCloseKey(hkey); 
} 

/* Providing a SID (security identifier)
 */
char *inetd_log_sid() {
    DWORD   size_sid = sizeof (inetd_log_user);
    PSID    user_security_identifier = NULL;
    static TCHAR user[256] = "";
    DWORD   size_user  =  255;
    TCHAR   domain[256];
	BYTE    sid_buf[4096];
    DWORD   size_domain = 255;
    SID_NAME_USE    type_sid;
    DWORD   sid_len;

    if (!inetd_log_init && !inetd_os_win95()) {
        /* Get security information of current user */
        ZeroMemory(user, sizeof (user));
        ZeroMemory(domain, sizeof (domain));
        ZeroMemory(sid_buf, size_sid);
        GetUserName(user, &size_user);
        if (LookupAccountName(0,
                              user,
                              &sid_buf,
                              &size_sid,
                              domain,
                              &size_domain,
                              &type_sid
                              )) {
            if (IsValidSid((PSID)(sid_buf))) {
                sid_len = GetLengthSid((PSID)(sid_buf));
        		CopySid(sid_len, inetd_log_user, sid_buf);
				EqualSid(inetd_log_user, sid_buf);
                inetd_log_init = INETD_TRUE;
            }
        }
    }
    return user;
}

#ifdef TEST_SID
void main() {
    inetd_log_sid();
}
#endif

#ifdef TEST_LOG

BOOL debug;

/* Event log tester
 */
void main() {
    debug = FALSE;
    inetd_log_message(INETD_LOG_SUCCESS, "Test Event - %m.");
}

#endif /* TEST_LOG */
