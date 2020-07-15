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
 * @(#)service.c: windows nt service routines
 * $Id: service.c,v 1.4 2002/02/07 08:44:01 cvs Exp $
 */

#include <inetd.h>
#include <io.h>
#include <fcntl.h>
#include "inetd_cpl.h"

/* internals */
SERVICE_STATUS          inetd_service_status;
SERVICE_STATUS_HANDLE   inetd_service_handle;
BOOL                    inetd_service_ready;

/* definations */
#ifndef RSP_SIMPLE_SERVICE
#define RSP_SIMPLE_SERVICE 1
#endif
#ifndef RSP_UNREGISTER_SERVICE
#define RSP_UNREGISTER_SERVICE 0
#endif

typedef DWORD (WINAPI *inetd_service_regfunc)(DWORD pid, DWORD type);

static LPCTSTR inetd_service_ntkey = TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");
static LPCTSTR inetd_service_95key = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\RunServices");

/* internals */
void inetd_service_register(char *file, DWORD types);
void inetd_service_deregister();
int inetd_service_dispatcher();
void inetd_service_set_console();

/* Install winNT service
 * Returns: 1 when installation succeeded while 0 failed
 */
int inetd_service_install(char *module) {
    SC_HANDLE   sch_service;
    SC_HANDLE   sch_manager;
    BOOL ret = 0;
    SERVICE_DESCRIPTION desc;
    /* have to show the console here for the
     * diagnostic or error reason: orignal class assumed
     * that we were using _main for entry (a console app).
     * This particular usage is a Windows app (no console),
     * so we need to create it. Using inetd_service_setcon with _main
     * is ok - does nothing, since you only get one console.
     */
    inetd_service_set_console();	
    if (inetd_os_win95()) {
        /* code added to install as Win95 service
         * Create a key for that application and insert values for
         * "EventMessageFile" and "TypesSupported"
         */
		HKEY key = 0;
		LONG res = ERROR_SUCCESS;
        if (RegCreateKey(HKEY_LOCAL_MACHINE, inetd_service_95key , &key) == ERROR_SUCCESS) {
            res = RegSetValueEx(key,                /* handle of key to set value for */
                                SERVICE_NAME,       /* address of value to set (NAME OF SERVICE) */
                                0,                  /* reserved */
                                REG_EXPAND_SZ,      /* flag for value type */
                                (CONST BYTE*)module,/* address of value data */
                                strlen(module) + 1);/* size of value data */
            RegCloseKey(key);
            ret = TRUE;
        }
    } else {
        sch_manager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
        if (sch_manager) {
            sch_service = CreateService(sch_manager,                /* SCManager database */
                                        TEXT(SERVICE_NAME),         /* name of service */
                                        TEXT(DISPLAY_NAME),         /* name to display */
                                        SERVICE_ALL_ACCESS,         /* desired access */
                                        SERVICE_WIN32_OWN_PROCESS,  /* service type */
                                        SERVICE_AUTO_START,         /* start type */
                                        SERVICE_ERROR_NORMAL,       /* error control type */
                                        module,                     /* service's binary */
                                        0,                          /* no load ordering group */
                                        0,                          /* no tag identifier */
                                        TEXT(DEPENDENCIES),         /* dependencies */
                                        0,                          /* LocalSystem account */
                                        0);                         /* no password */
            if (sch_service) {
                TCHAR desc_str[1024];
                LoadString(hinst, INETD_DESC, desc_str, 1024);
                desc.lpDescription = desc_str;
                ChangeServiceConfig2(sch_service, SERVICE_CONFIG_DESCRIPTION, &desc);
                CloseServiceHandle(sch_service);
                ret = TRUE;
            } else {
                inetd_error_display();
            }
            CloseServiceHandle(sch_manager);
        } else {
            inetd_error_display();
        }
		if (ret) {
			/* installation succeeded. Now register the message file */
			inetd_service_register(module,   /* the path to the application itself */
                                   EVENTLOG_ERROR_TYPE |
                                   EVENTLOG_WARNING_TYPE |
                                   EVENTLOG_INFORMATION_TYPE);   /* supported types */
                                  
            inetd_status_update();
		}
    }
    return ret;
}

/* Remove winNT service
 * Returns: 1 when removing succeeded while 0 failed
 */
int inetd_service_remove() {
    SC_HANDLE   sch_service;
    SC_HANDLE   sch_manager;
    BOOL ret = 0;
    /* have to show the console here for the
     * diagnostic or error reason: orignal class assumed
     * that we were using _main for entry (a console app).
     * This particular usage is a Windows app (no console),
     * so we need to create it. Using inetd_service_setcon with _main
     * is ok - does nothing, since you only get one console.
     */
    inetd_service_set_console();	
    if (inetd_os_win95()) { /* code added to install as Win95 service */
        HKEY key = 0;
        LONG res = ERROR_SUCCESS;
        if (RegCreateKey(HKEY_LOCAL_MACHINE, inetd_service_95key , &key) == ERROR_SUCCESS) {
            res = RegDeleteValue(key, SERVICE_NAME);
            RegCloseKey(key);
            ret = TRUE;
        }
    } else {
        sch_manager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
        if (sch_manager) {
            sch_service = OpenService(sch_manager, TEXT(SERVICE_NAME), SERVICE_ALL_ACCESS);
            if (sch_service) {
                /* try to stop the service */
                if (ControlService(sch_service, SERVICE_CONTROL_STOP, &inetd_service_status)) {
                    inetd_status_update();
                    Sleep(1000);
                    while (QueryServiceStatus(sch_service, &inetd_service_status)) {
                        if (inetd_service_status.dwCurrentState == SERVICE_STOP_PENDING) {
                            Sleep(1000);
                        } else
                            break;
                    }
                    if (inetd_service_status.dwCurrentState == SERVICE_STOPPED)
                        inetd_status_update();
                    else
                        inetd_error_display();
                }
                /* now remove the service */
                if (DeleteService(sch_service)) {
                    inetd_status_update();
                    ret = TRUE;
                } else {
                    inetd_error_display();
                }
                CloseServiceHandle(sch_service);
            } else {
                inetd_error_display();
            }
            CloseServiceHandle(sch_manager);
        } else {
            inetd_error_display();
        }
        if (ret)
            inetd_service_deregister();
    }
    return TRUE;
}

/* Start up winNT service
 * Returns: error status
 */
int inetd_service_start() {
    BOOL ret = 0;
    SC_HANDLE sch_manager = OpenSCManager(0,                        /* machine (0 == local) */
		                                  0,                        /* database (0 == default) */
                                          SC_MANAGER_ALL_ACCESS);   /* access required */
                                          
    if (sch_manager) {
        SC_HANDLE sch_service = OpenService(sch_manager,
                                            SERVICE_NAME,
                                            SERVICE_ALL_ACCESS);
        if (sch_service) {
            /* try to start the service */
            if (StartService(sch_service, 0, 0)) {
                inetd_status_update();
                Sleep(1000);
                while (QueryServiceStatus(sch_service, &inetd_service_status)) {
                    if (inetd_service_status.dwCurrentState == SERVICE_START_PENDING) {
                        Sleep(1000);
                    } else
                        break;
                }
                if (inetd_service_status.dwCurrentState == SERVICE_RUNNING) {
                    ret = TRUE;
                    inetd_status_update();
                } else
                    inetd_error_display();
            } else {
                /* StartService failed */
                inetd_error_display();
            }
            CloseServiceHandle(sch_service);
        } else {
            inetd_error_display();
        }
        CloseServiceHandle(sch_manager);
    } else {
        inetd_error_display();
    }
    return ret;
}

/* Stop running winNT service
 * Returns: error status
 */
int inetd_service_stop() {
    BOOL ret = 0;
    SC_HANDLE sch_manager = OpenSCManager(0,                        /* machine (0 == local) */
		                                  0,                        /* database (0 == default) */
                                          SC_MANAGER_ALL_ACCESS);   /* access required */
                                          
    if (sch_manager) {
        SC_HANDLE sch_service = OpenService(sch_manager,
                                            SERVICE_NAME,
                                            SERVICE_ALL_ACCESS);
        if (sch_service) {
            /* try to stop the service */
            if (ControlService(sch_service, SERVICE_CONTROL_STOP, &inetd_service_status)) {
                inetd_status_update();
                Sleep(1000);
                while (QueryServiceStatus(sch_service, &inetd_service_status)) {
                    if (inetd_service_status.dwCurrentState == SERVICE_STOP_PENDING) {
                        Sleep(1000);
                    } else
                        break;
                }
                if (inetd_service_status.dwCurrentState == SERVICE_STOPPED) {
                    ret = TRUE;
                    inetd_status_update();
                } else
                    inetd_error_display();
            }
            CloseServiceHandle(sch_service);
        } else {
            inetd_error_display();
        }
        CloseServiceHandle(sch_manager);
    } else {
        inetd_error_display();
    }
    return ret;
}

/* Create console for faceless apps if not already there
 */
void inetd_service_set_console() {
    if (!inetd_os_win95())
        return;
    if (!inetd_service_ready) {
        DWORD astds[3] = { STD_OUTPUT_HANDLE, STD_ERROR_HANDLE, STD_INPUT_HANDLE };
        FILE *atrgs[3] = { stdout, stderr, stdin };
        long hand;
        register int i;
        AllocConsole();	/* you only get 1 console. */
        /* lovely hack to get the standard io (printf, getc, etc) to the new console. Pretty much does what the
         * C lib does for us, but when we want it, and inside of a Window'd app.
         * The ugly look of this is due to the error checking (bad return values. Remove the if xxx checks if you like it that way.
         */
        for (i = 0; i < 3; i++) {
            hand = (long)GetStdHandle(astds[i]);
            if (hand != (long)INVALID_HANDLE_VALUE) {
                int osf = _open_osfhandle(hand, _O_TEXT);
                if (osf != -1) {
                    FILE *fp = _fdopen(osf, (astds[i] == STD_INPUT_HANDLE) ? "r" : "w");
                    if (fp!=0) {
                        *(atrgs[i]) = *fp;
                        setvbuf(fp, 0, _IONBF, 0);
                    }
                }
            }
        }
        inetd_service_ready = TRUE;
    }
}

/* Register event message environment
 * Accepts: event message file
 *          value type
 */
void inetd_service_register(char *file, DWORD types) {
    TCHAR key_name[256];
    HKEY key = 0;
    LONG ret = ERROR_SUCCESS;
    strcpy(key_name, inetd_service_ntkey);
    _tcscat(key_name, SERVICE_NAME);
    /* Create a key for that application and insert values for
     * "EventMessageFile" and "TypesSupported"
     */
    if (RegCreateKey(HKEY_LOCAL_MACHINE, key_name, &key) == ERROR_SUCCESS) {
        ret = RegSetValueEx(key,                        /* handle of key to set value for */
                            TEXT("EventMessageFile"),   /* address of value to set */
                            0,                          /* reserved */
                            REG_EXPAND_SZ,              /* flag for value type */
                            (CONST BYTE*)file,          /* address of value data */
                            strlen(file) + 1);          /* size of value data */
        /* Set the supported types flags. */
        ret = RegSetValueEx(key,                    /* handle of key to set value for */
                            TEXT("TypesSupported"), /* address of value to set */
                            0,                      /* reserved */
                            REG_DWORD,              /* flag for value type */
                            (CONST BYTE*)&types,    /* address of value data */
                            sizeof (DWORD));        /* size of value data */
        RegCloseKey(key);
    }
    /* Add the service to the "Sources" value */
    ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,  /* handle of open key */
                       inetd_service_ntkey,  /* address of name of subkey to open */
                       0,                   /* reserved */
                       KEY_ALL_ACCESS,      /* security access mask */
                       &key);               /* address of handle of open key */
    if (ret == ERROR_SUCCESS) {
        DWORD size;
        /* retrieve the size of the needed value */
        ret = RegQueryValueEx(key,              /* handle of key to query */
                              TEXT("Sources"),  /* address of name of value to query  */
                              0,                /* reserved */
                              0,                /* address of buffer for value type */
                              0,                /* address of data buffer */
                              &size);           /* address of data buffer size */
        if (ret == ERROR_SUCCESS) {
            DWORD type;
            LPBYTE buffer;
            register LPTSTR p;
            DWORD size_new = size + strlen(SERVICE_NAME) + 1;
            buffer = (LPBYTE)(GlobalAlloc(GPTR, size_new));
            ret = RegQueryValueEx(key,              /* handle of key to query */
                                  TEXT("Sources"),  /* address of name of value to query */
                                  0,                /* reserved */
                                  &type,            /* address of buffer for value type */
                                  buffer,           /* address of data buffer */
                                  &size);           /* address of data buffer size */
            if (ret == ERROR_SUCCESS) {
                if (type != REG_MULTI_SZ)
                    inetd_error_display();
                /* check whether this service is already a known source */
                p = (LPTSTR)(buffer);
                for (; *p; p += strlen(p)+1) {
                    if (strcmp(p, SERVICE_NAME) == 0)
                        break;
                }
                if (!(*p)) {
                    /* We're standing at the end of the stringarray
                     * and the service does still not exist in the "Sources".
                     * Now insert it at this point.
                     * Note that we have already enough memory allocated
                     * (see GlobalAlloc() above). We also don't need to append
                     * an additional '\0'. This is done in GlobalAlloc() above
                     * too.
                     */
                    strcpy(p, SERVICE_NAME);
                    /* OK - now store the modified value back into the
                     * registry.
                     */
                    ret = RegSetValueEx(key,            /* handle of key to set value for */
                                        TEXT("Sources"),/* address of value to set */
                                        0,              /* reserved */
                                        type,           /* flag for value type */
                                        buffer,         /* address of value data */
                                        size_new);      /* size of value data */
                }
            }
            GlobalFree((HGLOBAL)(buffer));
        }
        RegCloseKey(key);
    }
}

/* Deregister event message environment
 */
void inetd_service_deregister() {
    TCHAR key_name[256];
    HKEY key = 0;
    LONG ret = ERROR_SUCCESS;
    strcpy(key_name, inetd_service_ntkey);
    _tcscat(key_name, SERVICE_NAME);
    ret = RegDeleteKey(HKEY_LOCAL_MACHINE, key_name);
    /* now we have to delete the application from the "Sources" value too. */
    ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,  /* handle of open key */
                       inetd_service_ntkey, /* address of name of subkey to open */
                       0,                   /* reserved */
                       KEY_ALL_ACCESS,      /* security access mask */
                       &key);               /* address of handle of open key */
    if (ret == ERROR_SUCCESS) {
        DWORD size;
        /* retrieve the size of the needed value */
        ret = RegQueryValueEx(key,              /* handle of key to query */
                              TEXT("Sources"),  /* address of name of value to query */
                              0,                /* reserved */
                              0,                /* address of buffer for value type */
                              0,                /* address of data buffer */
                              &size             /* address of data buffer size */
                              );
        if (ret == ERROR_SUCCESS) {
            DWORD type;
            LPBYTE buffer = (LPBYTE)(GlobalAlloc(GPTR, size));
            LPBYTE buf_new = (LPBYTE)(GlobalAlloc(GPTR, size));
            register LPTSTR p;
            register LPTSTR pnew;
            BOOL need_save;
            ret = RegQueryValueEx(key,              /* handle of key to query */
                                  TEXT("Sources"),  /* address of name of value to query */
                                  0,                /* reserved */
                                  &type,            /* address of buffer for value type */
                                  buffer,           /* address of data buffer */
                                  &size             /* address of data buffer size */
                                  );
            if (ret == ERROR_SUCCESS) {
                if (type != REG_MULTI_SZ)
                    inetd_error_display();
                /* check whether this service is already a known source */
                p = (LPTSTR)(buffer);
                pnew = (LPTSTR)(buf_new);
                need_save = 0;  /* assume the value is already correct */
                for (; *p; p += strlen(p)+1, pnew += strlen(pnew)+1) {
                    /* except ourself: copy the source string into the destination */
                    if (strcmp(p, SERVICE_NAME) != 0)
                        strcpy(pnew, p);
                    else {
                        need_save = TRUE;       /* *this* application found */
                        size -= strlen(p)+1;	/* new size of value */
                    }
                }
                if (need_save) {
                    /* OK - now store the modified value back into the
                     * registry.
                     */
                    ret = RegSetValueEx(key,            /* handle of key to set value for */
                                        TEXT("Sources"),/* address of value to set */
                                        0,              /* reserved */
                                        type,           /* flag for value type */
                                        buf_new,        /* address of value data */
                                        size            /* size of value data */
                                        );
                }
            }
            GlobalFree((HGLOBAL)(buffer));
            GlobalFree((HGLOBAL)(buf_new));
        }
        RegCloseKey(key);
    }
}

int inetd_service_query() {
    SC_HANDLE sch_manager;
    SC_HANDLE sch_service;
    SERVICE_STATUS sstat;
    int res = INETD_FALSE;

    sch_manager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
    if (sch_manager) {
        sch_service = OpenService(sch_manager, TEXT(SERVICE_NAME), SERVICE_QUERY_STATUS);
        if (sch_service) {
            if (QueryServiceStatus(sch_service, &sstat)) {
                switch (sstat.dwCurrentState) {
                case SERVICE_STOP_PENDING:
                    res = INETD_SERVICE_STOPPING;
                    break;
                case SERVICE_STOPPED:
                    res = INETD_SERVICE_STOPPED;
                    break;
                case SERVICE_START_PENDING:
                    res = INETD_SERVICE_STARTING;
                    break;
                case SERVICE_RUNNING:
                    res = INETD_SERVICE_RUNNING;
                    break;
                default:
                    res = INETD_FALSE;
                }
            }
            CloseServiceHandle(sch_service);
        } else {
            if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
                res = INETD_SERVICE_UNINSTALLED;
            else {
                inetd_error_display();
                res = INETD_FALSE;
            }
        }
        CloseServiceHandle(sch_manager);
    } else {
        inetd_error_display();
        res = INETD_FALSE;
    }
    return res;
}

char *inetd_service_module() {
    static char module[MAX_PATH];
    SC_HANDLE sch_manager;
    SC_HANDLE sch_service;
    LPQUERY_SERVICE_CONFIG pqsc;
    char *res = NULL;

    sch_manager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
    if (sch_manager) {
        sch_service = OpenService(sch_manager, TEXT(SERVICE_NAME), SERVICE_QUERY_CONFIG);
        if (sch_service) {
            DWORD needed;
            int len;
            QueryServiceConfig(sch_service, NULL, 0, &needed);
            if (!needed) {
                inetd_error_display();
                return NULL;
            }
            pqsc = malloc(needed);
            if (!QueryServiceConfig(sch_service, pqsc, needed, &needed)) {
                free(pqsc);
                if (!needed) {
                    inetd_error_display();
                    return NULL;
                }
            }
            len = strlen(pqsc->lpBinaryPathName);
            if (len >= MAX_PATH) {
                memcpy(module, pqsc->lpBinaryPathName, len-1);
                module[len-1] = '\0';
            } else
                strcpy(module, pqsc->lpBinaryPathName);
            free(pqsc);
            res = module;
            CloseServiceHandle(sch_service);
        } else {
            if (GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST) {
                inetd_error_display();
            }
            res = NULL;
        }
        CloseServiceHandle(sch_manager);
    } else {
        inetd_error_display();
        res = NULL;
    }
    return res;
}
