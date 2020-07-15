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
 * @(#)status.c: inetd status property sheet
 * $Id: status.c,v 1.6 2002/02/22 05:30:04 zlv Exp $
 */

#include <inetd.h>
#include "inetd_cpl.h"

struct inetd_status {
    HWND    is_status;  /* status label */
    HWND    is_install; /* instatll button */
    HWND    is_remove;  /* remove button */
    HWND    is_start;   /* start button */
    HWND    is_stop;    /* stop button */
    HWND    is_module;  /* executable module */
    HWND    is_sheet;   /* property sheet */
    char    is_file[MAX_PATH];
} inetd_status_dlg;

struct inetd_status_detail {
    int status;
    int install;
    int remove;
    int start;
    int stop;
    unsigned int text;
} inetd_status_details[] = {
    { INETD_FALSE,                  INETD_FALSE,    INETD_FALSE,    INETD_FALSE,    INETD_FALSE,    INETD_STATUS_UNKNOWN },
    { INETD_SERVICE_UNINSTALLED,    INETD_TRUE,     INETD_FALSE,    INETD_FALSE,    INETD_FALSE,    INETD_STATUS_UNINSTALLED },
    { INETD_SERVICE_STOPPED,        INETD_FALSE,    INETD_TRUE,     INETD_TRUE,     INETD_FALSE,    INETD_STATUS_STOPPED },
    { INETD_SERVICE_RUNNING,        INETD_FALSE,    INETD_TRUE,     INETD_FALSE,    INETD_TRUE,     INETD_STATUS_RUNNING },
    { INETD_SERVICE_STARTING,       INETD_FALSE,    INETD_TRUE,     INETD_FALSE,    INETD_FALSE,    INETD_STATUS_STARTING },
    { INETD_SERVICE_STOPPING,       INETD_FALSE,    INETD_TRUE,     INETD_FALSE,    INETD_FALSE,    INETD_STATUS_STOPPING },
};

static int inetd_status_init(HWND hdlg);

LRESULT CALLBACK inetd_status_proc(HWND hdlg, UINT msg, UINT wparam, LONG lparam) {
    char module[MAX_PATH];

    switch (msg) {
        case WM_INITDIALOG:
            SetTimer(hdlg, INETD_STATUS_TIMER, 500, (TIMERPROC)inetd_status_proc);
            return (inetd_status_init(hdlg));
        case WM_TIMER:
            inetd_status_update();
            break;
        case WM_COMMAND:
            {
                /*
                if (HIWORD(wparam) == EN_CHANGE && 
                    LOWORD(wparam) == IDC_MODULE) {
                    PropSheet_Changed(inetd_status_dlg.is_sheet, hdlg);
                }
                */
                switch (LOWORD(wparam)) {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hdlg, 1);
                    break;
                case IDC_INSTALL:
                    if (!inetd_status_dlg.is_module)
                        inetd_error_prompt("Module item not initialized!");
                    else {
                        GetWindowText(inetd_status_dlg.is_module, module, MAX_PATH-1);
                        if (!module || !strlen(module))
                            inetd_error_prompt("Module not specified!");
                        else if (access(module, 0))
                            inetd_error_prompt("Module %s cannot be accessed!", module);
                        else
                            inetd_service_install(module);
                    }
                    break;
                case IDC_UNINSTALL:
                    inetd_service_remove();
                    break;
                case IDC_START:
                    inetd_service_start();
                    break;
                case IDC_STOP:
                    inetd_service_stop();
                    break;
				case IDC_BROWSE:
                    if (!inetd_status_dlg.is_module)
                        inetd_error_prompt("Module item not initialized!");
                    else {
                        char *file = inetd_service_browse(hdlg,
                                                          inetd_status_dlg.is_file,
                                                          sizeof (inetd_status_dlg.is_file));
                        if (file)
                            SetWindowText(inetd_status_dlg.is_module,
                                          file);
                    }
					break;
                }
            }
        case WM_PAINT:
            inetd_status_update();
            break;
        case WM_NOTIFY:
            {
                switch (((NMHDR FAR *) lparam)->code) {
                case PSN_HELP:
                    {
                        char help_file[MAX_PATH]; /* buffer for name of help file */
                        /* display help for the font properties page. */
                        LoadString(hinst, INETD_HELP, help_file, MAX_PATH);
                        WinHelp(((NMHDR FAR *) lparam)->hwndFrom, help_file,
                                HELP_FINDER, 0);
                        break;
                    }
                case PSN_APPLY:
                    {
                        /* save module path */
                        break;
                    }
                }
                break;
            }
        case WM_DESTROY:
            KillTimer(hdlg, INETD_STATUS_TIMER);
            break;
    }
    return 0;
}

void inetd_status_update() {
	int status;
    TCHAR text[30];

    if (!inetd_status_dlg.is_status ||
        !inetd_status_dlg.is_install ||
        !inetd_status_dlg.is_remove ||
        !inetd_status_dlg.is_start ||
        !inetd_status_dlg.is_stop)
        return;

    status = inetd_service_query();
    if (status > INETD_SERVICE_MAXSTAT)
        status = INETD_FALSE;
    LoadString(hinst, inetd_status_details[status].text, text, 30);
    SetWindowText(inetd_status_dlg.is_status, text);
    EnableWindow(inetd_status_dlg.is_install, inetd_status_details[status].install);
    EnableWindow(inetd_status_dlg.is_remove, inetd_status_details[status].remove);
    EnableWindow(inetd_status_dlg.is_start, inetd_status_details[status].start);
    EnableWindow(inetd_status_dlg.is_stop, inetd_status_details[status].stop);
}

int inetd_status_init(HWND hdlg) {
    char *temp;
    inetd_status_dlg.is_status = GetDlgItem(hdlg, IDC_STATUS);
    if (!inetd_status_dlg.is_status) {
        return INETD_FALSE;
    }
    inetd_status_dlg.is_install = GetDlgItem(hdlg, IDC_INSTALL);
    if (!inetd_status_dlg.is_install) {
        return INETD_FALSE;
    }
    inetd_status_dlg.is_remove = GetDlgItem(hdlg, IDC_UNINSTALL);
    if (!inetd_status_dlg.is_remove) {
        return INETD_FALSE;
    }
    inetd_status_dlg.is_start = GetDlgItem(hdlg, IDC_START);
    if (!inetd_status_dlg.is_start) {
        return INETD_FALSE;
    }
    inetd_status_dlg.is_stop = GetDlgItem(hdlg, IDC_STOP);
    if (!inetd_status_dlg.is_stop) {
        return INETD_FALSE;
    }
    inetd_status_dlg.is_module = GetDlgItem(hdlg, IDC_MODULE);
    if (!inetd_status_dlg.is_module) {
        return INETD_FALSE;
    }
    inetd_status_dlg.is_sheet = GetParent(hdlg);
    if (!inetd_status_dlg.is_sheet) {
        return INETD_FALSE;
    }
    temp = inetd_service_module();
    if (temp)
        strcpy(inetd_status_dlg.is_file, temp);
    else {
        if (GetModuleFileName(0, inetd_status_dlg.is_file, MAX_PATH-1) == 0) {
            inetd_error_display();
            return INETD_FALSE;
        }
        temp = strrchr(inetd_status_dlg.is_file, '\\');
        if (temp) strcpy(temp+1, "inetd.exe");
    }
    SetWindowText(inetd_status_dlg.is_module, inetd_status_dlg.is_file);

    inetd_status_update();
    return INETD_TRUE;
}

char *inetd_service_browse(HWND hparent, char *file, int size) {
    OPENFILENAME ofn;
    char filter[] = {
        'i', 'n', 'e', 't', 'd', ' ', 'f', 'i', 'l', 'e', 0,
        'i', 'n', 'e', 't', 'd', '.', 'e', 'x', 'e', 0,
        0,
        0,
    };

    ofn.lStructSize         = sizeof (OPENFILENAME);
    ofn.hwndOwner           = hparent;
    ofn.hInstance           = hinst;
    ofn.lpstrFilter         = filter;
    ofn.lpstrCustomFilter   = NULL;
    ofn.nMaxCustFilter      = 1;
    ofn.nFilterIndex        = 1;
    ofn.lpstrFile           = file;
    ofn.nMaxFile            = size;
    ofn.lpstrFileTitle      = NULL;
    ofn.nMaxFileTitle       = 0;
    ofn.lpstrInitialDir     = NULL;
    ofn.lpstrTitle          = "Select Internet Daemon Binary";
    ofn.nFileOffset         = 0;
    ofn.nFileExtension      = 0;
    ofn.lpstrDefExt         = NULL;
    ofn.lCustData           = 0;
    ofn.lpfnHook            = NULL;
    ofn.lpTemplateName      = NULL;
    ofn.Flags               = OFN_SHOWHELP | OFN_EXPLORER;
    
    /* call the common dialog function. */
    if (GetOpenFileName(&ofn)) {
        return ofn.lpstrFile;
    }
    return NULL;
}
