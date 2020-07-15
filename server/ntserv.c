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
 * @(#)ntserv.c: windows nt service routines
 * $Id: ntserv.c,v 1.3 2002/02/07 08:44:02 cvs Exp $
 */

#include <inetd.h>
#include "ntserv.h"
#include "resource.h"
#include <io.h>
#include <fcntl.h>

/* externals */
HANDLE                  stopevent = 0;
BOOL                    debug = 0;

/* internals */

SERVICE_STATUS          sstat;
SERVICE_STATUS_HANDLE   sstathdl;
BOOL                    conready;
DWORD                   ctrlsacpted = SERVICE_ACCEPT_STOP;
BOOL                    instance = 0;

/* definations */
#ifndef RSP_SIMPLE_SERVICE
#define RSP_SIMPLE_SERVICE 1
#endif
#ifndef RSP_UNREGISTER_SERVICE
#define RSP_UNREGISTER_SERVICE 0
#endif

typedef DWORD (WINAPI *ntserv_regfunc)(DWORD pid, DWORD type);

static LPCTSTR ntserv_regkey = TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");
static LPCTSTR ntserv_95key = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\RunServices");

/* callbacks */
BOOL WINAPI control_handler(DWORD ctrl_type);
void WINAPI service_main(DWORD uargc, LPTSTR *uargv);
void WINAPI service_ctrl(DWORD ctrl_code);
LRESULT CALLBACK faceless_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

/* internals */
void ntserv_register(char *file, DWORD types);
void ntserv_deregister();
int ntserv_dispatcher();
void ntserv_set_console();
void ntserv_run(DWORD argc, LPTSTR * argv);
void ntserv_stop();
void ntserv_usage();

#if (!defined (TEST_LOG) && !defined (TEST_SID))

/* WinNT service main entry
 * Accepts: number of arguments
 *          ** pointer to command arguments
 *          ** pointer to environment parameters
 * Returns: error status
 */
int _tmain(int argc, TCHAR* argv[], TCHAR* envp[]) {
    DWORD Argc;
    LPTSTR *Argv;
    BOOL (*fnc)() = ntserv_dispatcher;
    debug = 0;
    conready = 0;
    instance = TRUE;
#define NEXT_ARG ((((*Argv)[2]) == TEXT('\0')) ? (--Argc, *++Argv) : (*Argv)+2)
#ifdef UNICODE
    Argv = CommandLineToArgvW(GetCommandLineW(), &Argc );
#else
    Argc = (DWORD)argc;
    Argv = argv;
#endif
    /* SERVICE_STATUS members that rarely change */
    sstat.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    sstat.dwServiceSpecificExitCode = 0;
    while (++Argv, --Argc) {
        if (Argv[0][0] == TEXT('-')) {
            switch (Argv[0][1]) {
            case TEXT('i'): /* install the service */
                fnc = ntserv_install;
                break;
            /*case TEXT('l'): /* login-account (only useful with -i) */
                /*username = NEXT_ARG;
                break;
            case TEXT('p'): /* password (only useful with -i) */
                /*passwd = NEXT_ARG;
                break;*/
            case TEXT('u'): /* uninstall the service */
                fnc = ntserv_remove;
                break;
            case TEXT('s'): /* start the service */
                fnc = ntserv_start;
                break;
            case TEXT('e'): /* end the service */
                fnc = ntserv_end;
                break;
            case TEXT('d'): /* debug the service */
            case TEXT('f'): /* faceless non-service (Win95) mode */
#ifdef UNICODE
                GlobalFree(HGLOBAL)Argv);
#endif
                debug = TRUE;
                /* pass original parameters to ntserv_debug() */
                return ntserv_debug(argc, argv, (Argv[0][1] == TEXT('f')));   /* faceless non-service (Win95) mode */
            case TEXT('h'):
                ntserv_usage();
            }
		}
	}
#ifdef UNICODE
    GlobalFree(HGLOBAL)Argv);
#endif
    /* if Win95, run as faceless app. */
    if (fnc == ntserv_dispatcher && inetd_os_win95()) {
        /* act as if -f was passed anyways. */
        debug = TRUE;
        return ntserv_debug(argc, argv, TRUE);
    }
    return (*fnc)();
}

#endif

/* Start service control dispatcher
 * Returns: 1 when success while 0 when failure
 */
int ntserv_dispatcher() {
    /* default implementation creates a single threaded service.
     * override this method and provide more table entries for
     * a multithreaded service (one entry for each thread).
     */
    SERVICE_TABLE_ENTRY dispatch_table[] = {
        { (SERVICE_NAME), (LPSERVICE_MAIN_FUNCTION)service_main },
        { 0, 0 }
    };
    BOOL ret = StartServiceCtrlDispatcher(dispatch_table);
    if (!ret) {
        inetd_log_message(debug?INETD_LOG_DEBUG:INETD_LOG_FAILURE, "ntserv_dispatcher: StartServiceCtrlDispatcher - %m");
    }
    return ret;
}

/* WinNT service usage information
 */
void ntserv_usage() {
    printf("%s -i          to install the %s\n", APP_NAME, SERVICE_NAME);
    printf("%s -u          to uninstall the %s\n", APP_NAME, SERVICE_NAME);
    printf("%s -d          to debug the %s\n", APP_NAME, SERVICE_NAME);
    printf("%s -f          to run faceless %s on win95\n", APP_NAME, SERVICE_NAME);
    printf("%s -s          to start installed %s\n", APP_NAME, SERVICE_NAME);
    printf("%s -e          to stop installed %s\n", APP_NAME, SERVICE_NAME);
    printf("%s -h          to view this help infomation\n", APP_NAME, SERVICE_NAME);
    exit(0);
}

/* Install winNT service
 * Returns: 1 when installation succeeded while 0 failed
 */
int ntserv_install() {
    SC_HANDLE   sch_service;
    SC_HANDLE   sch_manager;
    TCHAR path[1024];
    BOOL ret = 0;
    SERVICE_DESCRIPTION desc;
    /* have to show the console here for the
     * diagnostic or error reason: orignal class assumed
     * that we were using _main for entry (a console app).
     * This particular usage is a Windows app (no console),
     * so we need to create it. Using ntserv_setcon with _main
     * is ok - does nothing, since you only get one console.
     */
    ntserv_set_console();	
    if (GetModuleFileName(0, path, 1023) == 0) {
        inetd_log_message(INETD_LOG_DEBUG, "ntserv_install: GetModuleFileName -%m");
        return 0;
    }
    if (inetd_os_win95()) {
        /* code added to install as Win95 service
         * Create a key for that application and insert values for
         * "EventMessageFile" and "TypesSupported"
         */
		HKEY key = 0;
		LONG res = ERROR_SUCCESS;
        if (RegCreateKey(HKEY_LOCAL_MACHINE, ntserv_95key , &key) == ERROR_SUCCESS) {
            res = RegSetValueEx(key,                /* handle of key to set value for */
                                SERVICE_NAME,       /* address of value to set (NAME OF SERVICE) */
                                0,                  /* reserved */
                                REG_EXPAND_SZ,      /* flag for value type */
                                (CONST BYTE*)path,  /* address of value data */
                                strlen(path) + 1    /* size of value data */
                               );
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
                                        path,                       /* service's binary */
                                        0,                          /* no load ordering group */
                                        0,                          /* no tag identifier */
                                        TEXT(DEPENDENCIES),         /* dependencies */
                                        0,                          /* LocalSystem account */
                                        0);                         /* no password */
            if (sch_service) {
                TCHAR desc_str[1024];
                LoadString(NULL, INETD_DESC, desc_str, 1024);
                desc.lpDescription = desc_str;
                ChangeServiceConfig2(sch_service, SERVICE_CONFIG_DESCRIPTION, &desc);
                CloseServiceHandle(sch_service);
                ret = TRUE;
            } else {
                inetd_log_message(INETD_LOG_DEBUG, "ntserv_install: CretaeService - %m");
            }
            CloseServiceHandle(sch_manager);
        } else {
            inetd_log_message(INETD_LOG_DEBUG, "ntserv_install: openSCManager - %m");
        }
		if (ret) {
			/* installation succeeded. Now register the message file */
			ntserv_register(path,   /* the path to the application itself */
                            EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE   /* supported types */
                           );
            inetd_log_message(INETD_LOG_DEBUG, "%s installed.", TEXT(SERVICE_NAME));
		}

    }
    return ret;
}

/* Remove winNT service
 * Returns: 1 when removing succeeded while 0 failed
 */
int ntserv_remove() {
    SC_HANDLE   sch_service;
    SC_HANDLE   sch_manager;
    BOOL ret = 0;
    /* have to show the console here for the
     * diagnostic or error reason: orignal class assumed
     * that we were using _main for entry (a console app).
     * This particular usage is a Windows app (no console),
     * so we need to create it. Using ntserv_setcon with _main
     * is ok - does nothing, since you only get one console.
     */
    ntserv_set_console();	
    if (inetd_os_win95()) { /* code added to install as Win95 service */
        HKEY key = 0;
        LONG res = ERROR_SUCCESS;
        if (RegCreateKey(HKEY_LOCAL_MACHINE, ntserv_95key , &key) == ERROR_SUCCESS) {
            res = RegDeleteValue(key, SERVICE_NAME);
            RegCloseKey(key);
            ret = TRUE;
        }
    } else {
        sch_manager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
        if (sch_manager) {
            sch_service = OpenService(sch_manager, TEXT(SERVICE_NAME), SERVICE_ALL_ACCESS);
            if (sch_service) {
                inetd_log_message(INETD_LOG_DEBUG, "Stopping %s.", TEXT(DISPLAY_NAME));
                /* try to stop the service */
                if (ControlService(sch_service, SERVICE_CONTROL_STOP, &sstat)) {
                    Sleep(1000);
                    printf(".");
                    while (QueryServiceStatus(sch_service, &sstat)) {
                        if (sstat.dwCurrentState == SERVICE_STOP_PENDING) {
                            Sleep(1000);
                            printf(".");
                        } else
                            break;
                    }
                    printf("\n");
                    if (sstat.dwCurrentState == SERVICE_STOPPED)
                        inetd_log_message(INETD_LOG_DEBUG, "%s stopped.", TEXT(DISPLAY_NAME));
                    else
                        inetd_log_message(INETD_LOG_DEBUG, "ntserv_remove: ControlService - %m");
                }
                /* now remove the service */
                if (DeleteService(sch_service)) {
                    inetd_log_message(INETD_LOG_DEBUG, "%s removed.", TEXT(DISPLAY_NAME));
                    ret = TRUE;
                } else {
                    inetd_log_message(INETD_LOG_DEBUG, "ntserv_remove: DeleteService failure - %m");
                }
                CloseServiceHandle(sch_service);
            } else {
                inetd_log_message(INETD_LOG_DEBUG, "ntserv_remove: OpenService - %m");
            }
            CloseServiceHandle(sch_manager);
        } else {
            inetd_log_message(INETD_LOG_DEBUG, "ntserv_remove: OpenSCManager - %m");
        }
        if (ret)
            ntserv_deregister();
    }
    return TRUE;
}

/* Debug winNT service
 * Accepts: number of aguments
 *          ** pointer to arguments
 *          if faceless for win95
 * Returns: error status
 */
int ntserv_debug(int argc, char **argv, int faceless) {
    DWORD uargc;
    LPTSTR *uargv;
	ntserv_regfunc fncptr = 0;
#ifdef UNICODE
    uargv = CommandLineToArgvW(GetCommandLineW(), &(uargc));
#else
    uargc   = (DWORD)argc;
    uargv = argv;
#endif
	if (!faceless) {    /* no faceless, so give it a face. */
        ntserv_set_console(); /* make the console for debugging */
        inetd_log_message(INETD_LOG_DEBUG, "Debugging %s.", TEXT(DISPLAY_NAME));
        SetConsoleCtrlHandler(control_handler, TRUE);
    }
    /*if Win95, register server */
    if (faceless /*&& inetd_os_win95()*/) {
        WNDCLASS wndclass;
        ATOM atom;
        HWND hwnd;
        HMODULE module;
        memset(&wndclass, 0, sizeof (WNDCLASS));
        wndclass.lpfnWndProc = faceless_wndproc;
        wndclass.hInstance = (HINSTANCE)(GetModuleHandle(0));
        wndclass.lpszClassName = TEXT("RRL_faceless_wndproc");
        atom = RegisterClass(&wndclass);
        hwnd = CreateWindow(wndclass.lpszClassName, TEXT(""), 0, 0, 0, 0, 0, 0, 0, wndclass. hInstance, 0);
        module = GetModuleHandle(TEXT("kernel32.dll"));
        /* punch F1 on "RegisterServiceProcess" for what it does and when to use it. */
        fncptr = (ntserv_regfunc)GetProcAddress(module, "RegisterServiceProcess");
        if (fncptr != 0)
            (*fncptr)(0, RSP_SIMPLE_SERVICE);
    }
    ntserv_run(uargc, uargv);
#ifdef UNICODE
    GlobalFree((HGLOBAL)uargv);
#endif
    if (fncptr != 0)     /* if it's there, remove it: our run is over */
        (*fncptr)(0, RSP_UNREGISTER_SERVICE);
    return TRUE;
}

/* Start up winNT service
 * Accepts: number of aguments
 *          ** pointer to arguments
 * Returns: error status
 */
int ntserv_start(DWORD uargc, LPTSTR *uargv) {
    BOOL ret = 0;
    SC_HANDLE sch_manager = OpenSCManager(0,                    /* machine (0 == local) */
		                                  0,                    /* database (0 == default) */
                                          SC_MANAGER_ALL_ACCESS /* access required */
                                          );
    if (sch_manager) {
        SC_HANDLE sch_service = OpenService(sch_manager,
                                            SERVICE_NAME,
                                            SERVICE_ALL_ACCESS);
        if (sch_service) {
            /* try to start the service */
            inetd_log_message(INETD_LOG_DEBUG, "Starting %s.", DISPLAY_NAME);
            if (StartService(sch_service, 0, 0)) {
                Sleep(1000);
                printf(".");
                while (QueryServiceStatus(sch_service, &sstat)) {
                    if (sstat.dwCurrentState == SERVICE_START_PENDING) {
                        Sleep(1000);
                        printf(".");
                    } else
                        break;
                }
                printf("\n");
                if (sstat.dwCurrentState == SERVICE_RUNNING) {
                    ret = TRUE;
                    inetd_log_message(INETD_LOG_DEBUG, "%s started.", DISPLAY_NAME);
                } else
                    inetd_log_message(INETD_LOG_DEBUG, "%s failed to start.", DISPLAY_NAME);
            } else {
                /* StartService failed */
                inetd_log_message(INETD_LOG_DEBUG, "ntserv_start: StartService - %m");
            }
            CloseServiceHandle(sch_service);
        } else {
            inetd_log_message(INETD_LOG_DEBUG, "ntserv_start: OpenService - %m");
        }
        CloseServiceHandle(sch_manager);
    } else {
        inetd_log_message(INETD_LOG_DEBUG, "ntserv_start: OpenSCManager - %m");
    }
    return ret;
}

/* Stop running winNT service
 * Returns: error status
 */
int ntserv_end() {
    BOOL ret = 0;
    SC_HANDLE sch_manager = OpenSCManager(0,                    /* machine (0 == local) */
		                                  0,                    /* database (0 == default) */
                                          SC_MANAGER_ALL_ACCESS /* access required */
                                          );
    if (sch_manager) {
        SC_HANDLE sch_service = OpenService(sch_manager,
                                            SERVICE_NAME,
                                            SERVICE_ALL_ACCESS);
        if (sch_service) {
            /* try to stop the service */
            if (ControlService(sch_service, SERVICE_CONTROL_STOP, &sstat)) {
                inetd_log_message(INETD_LOG_DEBUG, "Stopping %s.", DISPLAY_NAME);
                printf(".");
                Sleep(1000);
                while (QueryServiceStatus(sch_service, &sstat)) {
                    if (sstat.dwCurrentState == SERVICE_STOP_PENDING) {
                        printf(".");
                        Sleep(1000);
                    } else
                        break;
                }
                printf("\n");
                if (sstat.dwCurrentState == SERVICE_STOPPED) {
                    ret = TRUE;
                    inetd_log_message(INETD_LOG_DEBUG, "%s stopped.", DISPLAY_NAME);
                } else
                    inetd_log_message(INETD_LOG_DEBUG, "%s failed to stop.", DISPLAY_NAME);
            }
            CloseServiceHandle(sch_service);
        } else {
            inetd_log_message(INETD_LOG_DEBUG, "ntserv_end: OpenService - %m");
        }
        CloseServiceHandle(sch_manager);
    } else {
        inetd_log_message(INETD_LOG_DEBUG, "ntserv_end: OpenSCManager - %m");
    }
    return ret;
}

/* WinNT service running loop
 * Returns: number of arguments
 *          ** pointer to arguments
 */
void ntserv_run(DWORD argc, LPTSTR *argv) {
    /* report to the SCM that we're about to start */
    ntserv_create();
	if (!ntserv_init())
        return;
    if (!debug)
        inetd_log_message(INETD_LOG_INFO, "%s started.", DISPLAY_NAME);
    ntserv_report(SERVICE_START_PENDING, 0, 3000);
    stopevent = CreateEvent(0, TRUE, 0, 0);
    /* You might do some more initialization here.
     * Parameter processing for instance ...
     */
    /* report SERVICE_RUNNING immediately before you enter the main-loop
	 * DON'T FORGET THIS!
     */
    ntserv_report(SERVICE_RUNNING, 0, 3000);
    /* enter main-loop
     * If the ntserv_stop() method sets the event, then we will break out of
     * this loop.
     */
    while (WaitForSingleObject(stopevent, 10) != WAIT_OBJECT_0) {
        if (!ntserv_work())
            break;
    }
    if (stopevent)
        CloseHandle(stopevent);
    ntserv_destroy();
}

/* Stop winNT service
 */
void ntserv_stop() {
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    ntserv_fin();
    if (!debug)
        inetd_log_message(INETD_LOG_INFO, "%s stopped.", DISPLAY_NAME);
    ntserv_report(SERVICE_STOP_PENDING, 11000, 3000);
    if (stopevent)
        SetEvent(stopevent);
}

/* Create console for faceless apps if not already there
 */
void ntserv_set_console() {
    if (!conready) {
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
        conready = TRUE;
    }
}

/* Report winNT service status to service manager
 * Accepts: current status
 *          exit code
 *          wait hint
 * Returns: result of setting service status
 */
int ntserv_report(DWORD current, DWORD ecode, DWORD waithint) {
    static DWORD chk_point = 1;
    BOOL res = TRUE;
    if (!debug) {/* when debugging we don't report to the SCM */
        if (current == SERVICE_START_PENDING)
            sstat.dwControlsAccepted = 0;
        else
            sstat.dwControlsAccepted = /*SERVICE_ACCEPT_STOP*/ctrlsacpted;
        sstat.dwCurrentState = current;
        sstat.dwWin32ExitCode = NO_ERROR;
        sstat.dwWaitHint = waithint;
        /* added code to support error exiting */
        sstat.dwServiceSpecificExitCode = ecode;
        if (ecode != 0)
            sstat.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        if ((current == SERVICE_RUNNING) ||
            (current == SERVICE_STOPPED))
            sstat.dwCheckPoint = 0;
        else
            sstat.dwCheckPoint = ++chk_point;
        /* Report the status of the service to the service control manager.
         */
        if (!(res = SetServiceStatus(sstathdl, &sstat))) {
            inetd_log_message(debug?INETD_LOG_DEBUG:INETD_LOG_ERROR, "SetServiceStatus Error - %m");
        }
    }
    return res;
}

/* Register event message environment
 * Accepts: event message file
 *          value type
 */
void ntserv_register(char *file, DWORD types) {
    TCHAR key_name[256];
    HKEY key = 0;
    LONG ret = ERROR_SUCCESS;
    strcpy(key_name, ntserv_regkey);
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
                            strlen(file) + 1            /* size of value data */
                            );
        /* Set the supported types flags. */
        ret = RegSetValueEx(key,                        /* handle of key to set value for */
                            TEXT("TypesSupported"),     /* address of value to set */
                            0,                          /* reserved */
                            REG_DWORD,                  /* flag for value type */
                            (CONST BYTE*)&types,        /* address of value data */
                            sizeof (DWORD)              /* size of value data */
                            );
        RegCloseKey(key);
    }
    /* Add the service to the "Sources" value */
    ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,  /* handle of open key */
                       ntserv_regkey,       /* address of name of subkey to open */
                       0,                   /* reserved */
                       KEY_ALL_ACCESS,      /* security access mask */
                       &key                 /* address of handle of open key */
                       );
    if (ret == ERROR_SUCCESS) {
        DWORD size;
        /* retrieve the size of the needed value */
        ret = RegQueryValueEx(key,              /* handle of key to query */
                              TEXT("Sources"),  /* address of name of value to query  */
                              0,                /* reserved */
                              0,                /* address of buffer for value type */
                              0,                /* address of data buffer */
                              &size             /* address of data buffer size */
                              );
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
                                  &size             /* address of data buffer size */
                                  );
            if (ret == ERROR_SUCCESS) {
                if (type != REG_MULTI_SZ)
                    inetd_log_message(INETD_LOG_DEBUG, "ntserv_register: not REG_MULTI_SZ - %m");
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
                                        size_new        /* size of value data */
                                        );
                }
            }
            GlobalFree((HGLOBAL)(buffer));
        }
        RegCloseKey(key);
    }
}

/* Deregister event message environment
 */
void ntserv_deregister() {
    TCHAR key_name[256];
    HKEY key = 0;
    LONG ret = ERROR_SUCCESS;
    strcpy(key_name, ntserv_regkey);
    _tcscat(key_name, SERVICE_NAME);
    ret = RegDeleteKey(HKEY_LOCAL_MACHINE, key_name);
    /* now we have to delete the application from the "Sources" value too. */
    ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,  /* handle of open key */
                       ntserv_regkey,       /* address of name of subkey to open */
                       0,                   /* reserved */
                       KEY_ALL_ACCESS,      /* security access mask */
                       &key                 /* address of handle of open key */
                       );
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
                    inetd_log_message(INETD_LOG_DEBUG, "ntserv_register: not REG_MULTI_SZ - %m");
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

/* winNT servie main
 * Accepts: number of arguments
 *          arguments
 */
void WINAPI service_main(DWORD uargc, LPTSTR *uargv) {
    /* register our service control handler:
     */
    sstathdl = RegisterServiceCtrlHandler(TEXT(SERVICE_NAME), service_ctrl);
    if (sstathdl) {
        if (ntserv_report(SERVICE_START_PENDING,  /* service state */
                          NO_ERROR,               /* exit code */
                          3000))                  /* wait hint */
            ntserv_run(uargc, uargv);
    }
    if (sstathdl)
        (void)ntserv_report(SERVICE_STOPPED, GetLastError(), 0);
}

/* Control winNT service
 * Accepts: control code
 */
void WINAPI service_ctrl(DWORD ctrl_code) {
    /* Handle the requested control code.
     */
	switch (ctrl_code) {
    case SERVICE_CONTROL_STOP:
        /* Stop the service.
         *
         * SERVICE_STOP_PENDING should be reported before
         * setting the Stop Event - hServerStopEvent - in
         * ntserv_stop().  This avoids a race condition
         * which may result in a 1053 - The Service did not respond...
         * error.
         */
        sstat.dwCurrentState = SERVICE_STOP_PENDING;
        ntserv_stop();
        break;
    case SERVICE_CONTROL_PAUSE:
        sstat.dwCurrentState = SERVICE_PAUSE_PENDING;
        ntserv_pause();
        break;
    case SERVICE_CONTROL_CONTINUE:
        sstat.dwCurrentState = SERVICE_CONTINUE_PENDING;
        ntserv_continue();
        break;
    case SERVICE_CONTROL_SHUTDOWN:
        ntserv_shutdown();
        break;
    case SERVICE_CONTROL_INTERROGATE:
        /* Update the service status. */
        ntserv_report(sstat.dwCurrentState, NO_ERROR, 3000);
        break;
    default:
        /* invalid control code */
        break;
    }
}

/* Control console
 * Accepts: control type
 * Returns: whether success
 */
BOOL WINAPI control_handler(DWORD ctrl_type) {
    switch (ctrl_type) {
    case CTRL_BREAK_EVENT:  /* use Ctrl+C or Ctrl+Break to simulate */
    case CTRL_C_EVENT:      /* SERVICE_CONTROL_STOP in debug mode */
        inetd_log_message(INETD_LOG_DEBUG, "Stopping %s.", TEXT(DISPLAY_NAME));
        ntserv_stop();
        return TRUE;
        break;
    }
    return 0;
}

/* Faceless window procedure for usage within Win95 (mostly), but can be invoked under NT by using -f
 * Accepts: window handle
 *          windows message
 *          message's wparam
 *          message's lparam
 * Returns: return result
 */
LRESULT CALLBACK faceless_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_QUERYENDSESSION || msg == WM_ENDSESSION || msg == WM_QUIT) {
        if ((!lparam) || msg == WM_QUIT) {
            DestroyWindow(hwnd);    /* kill me */
            if (instance)
                ntserv_stop();      /* stop me. */
            return TRUE;
        }
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}
