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
 * @(#)attrib.c: inetd service attributes dialog
 * $Id: attrib.c,v 1.7 2002/01/28 05:24:06 cvs Exp $
 */

#include <inetd.h>
#include "inetd_cpl.h"

#define INETD_ATTRIB_MAX        MAX_PATH

struct inetd_attrib {
    HWND    ia_service;
    HWND    ia_user;
    HWND    ia_program;
    HWND    ia_cmdline;
    char    ia_file[MAX_PATH];
    struct inetd_serv *ia_serv;
} inetd_attrib_dlg;

#define inetd_attrib_validate(x, w)                     \
    do {                                                \
        if (!strlen(x)) {                               \
            inetd_error_prompt("Invalid %s value!", w); \
            return INETD_FALSE;                         \
        }                                               \
    } while (0)

static void inetd_attrib_center(HWND hwnd);
static char *inetd_attrib_browse(HWND hparent, char *file, int size);
static int inetd_attrib_init(HWND hdlg);
static int inetd_attrib_store(HWND hdlg);

LRESULT CALLBACK inetd_attrib_proc(HWND hdlg, UINT msg, UINT wparam, LONG lparam) {
    switch (msg) {
        case WM_INITDIALOG:
            inetd_attrib_dlg.ia_serv = (struct inetd_serv *)lparam;
            return (inetd_attrib_init(hdlg));
        case WM_COMMAND:
            {
                switch (LOWORD(wparam)) {
                case IDOK:
                    if (inetd_attrib_store(hdlg))
                        EndDialog(hdlg, INETD_ATTRIB_RETURN);
                    else
                        return FALSE;
                    break;
                case IDCANCEL:
                    EndDialog(hdlg, 1);
                    break;
                case IDC_PROGRAM:
                    if (!inetd_attrib_dlg.ia_program)
                        inetd_error_prompt("Program item not initialized!");
                    else {
                        char *file;
                        char *start, *end;
                        char command[MAX_PATH];
                        GetWindowText(inetd_attrib_dlg.ia_program,
                                      inetd_attrib_dlg.ia_file,
                                      INETD_ATTRIB_MAX);
                        file = inetd_attrib_browse(hdlg,
                                                   inetd_attrib_dlg.ia_file,
                                                   sizeof (inetd_attrib_dlg.ia_file));
                        if (file) {
                            start = strrchr(file, '\\');
                            end = strrchr(file, '.');
                            if (start && end) {
                                memcpy(command, start+1, end-start-1);
                                command[end-start-1] = '\0';
                                SetWindowText(inetd_attrib_dlg.ia_cmdline, command);
                            }
                            SetWindowText(inetd_attrib_dlg.ia_program, file);
                        }
                    }
                    break;
                }
                case IDC_MODULE:
                    break;
                case IDC_SERVICE:
                    break;
            }
        case WM_DESTROY:
            break;
    }
    return 0;
}

void inetd_attrib_center(HWND hwnd) {
    HWND  hwndParent;
    int   xNew, yNew;
    int   cxChild, cyChild;
    int   cxParent, cyParent;
    int   cxScreen, cyScreen;
    RECT  rcChild, rcParent;
    HDC   hdc;

    hwndParent = GetParent(hwnd);
    if (!hwndParent)
        return;

    /* Get the Height and Width of the child window */
    GetWindowRect(hwnd, &rcChild);
    cxChild = rcChild.right - rcChild.left;
    cyChild = rcChild.bottom - rcChild.top;
    /* Get the Height and Width of the parent window */
    GetWindowRect(hwndParent, &rcParent);
    cxParent = rcParent.right - rcParent.left;
    cyParent = rcParent.bottom - rcParent.top;
    /* Get the display limits */
    hdc = GetDC(hwnd);
    if (hdc == NULL) {
        /* major problems - move window to 0,0 */
        xNew = yNew = 0;
    } else {
        cxScreen = GetDeviceCaps(hdc, HORZRES);
        cyScreen = GetDeviceCaps(hdc, VERTRES);
        ReleaseDC(hwnd, hdc);
        if (hwndParent == NULL) {
            cxParent = cxScreen;
            cyParent = cyScreen;
            SetRect(&rcParent, 0, 0, cxScreen, cyScreen);
        }
        /* Calculate new X position, then adjust for screen */
        xNew = rcParent.left + ((cxParent - cxChild) / 2);
        if (xNew < 0) {
            xNew = 0;
        } else if ((xNew + cxChild) > cxScreen) {
            xNew = cxScreen - cxChild; 
        }
        /* Calculate new Y position, then adjust for screen */
        yNew = rcParent.top  + ((cyParent - cyChild) / 2);
        if (yNew < 0) {
            yNew = 0;
        } else if ((yNew + cyChild) > cyScreen) {
            yNew = cyScreen - cyChild;
        }  
    }
    SetWindowPos(hwnd, NULL, xNew, yNew, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER); 
}

char *inetd_attrib_browse(HWND hparent, char *file, int size) {
    OPENFILENAME ofn;
    char filter[] = {
        's', 'e', 'r', 'v', 'i', 'c', 'e', ' ',
        'p', 'r', 'o', 'g', 'r', 'a', 'm', 0,
        '*', '.', 'e', 'x', 'e', 0,
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
    ofn.lpstrTitle          = "Select Service Program";
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

void inetd_attrib_display(HWND hdlg, struct inetd_serv *serv) {
    int id;
    CheckRadioButton(hdlg, IDC_STREAM, IDC_DGRAM,
                     serv->is_socktype == SOCK_DGRAM ? IDC_DGRAM : IDC_STREAM);
    CheckRadioButton(hdlg, IDC_TCP, IDC_UDP,
                     (serv->is_proto && !strcmp(serv->is_proto, "udp")) ? IDC_UDP : IDC_TCP);
    CheckRadioButton(hdlg, IDC_NOWAIT, IDC_YESWAIT, serv->is_wait ? IDC_YESWAIT : IDC_NOWAIT);
    switch (serv->is_type) {
    case INETD_MUX_TYPE:
        id = IDC_TCPMUX;
        break;
    case INETD_MUXPLUS_TYPE:
        id = IDC_MUXPLUS;
        break;
    case INETD_NORM_TYPE:
    default:
        id = IDC_NORMAL;
        break;
    }
    CheckRadioButton(hdlg, IDC_NORMAL, IDC_MUXPLUS, id);
    if (serv->is_server)
        SetWindowText(inetd_attrib_dlg.ia_program, serv->is_server);
    if (serv->is_cmdline)
        SetWindowText(inetd_attrib_dlg.ia_cmdline, serv->is_cmdline);
    if (serv->is_service)
        SetWindowText(inetd_attrib_dlg.ia_service, serv->is_service);
    if (serv->is_user)
        SetWindowText(inetd_attrib_dlg.ia_user, serv->is_user);
    else
        SetWindowText(inetd_attrib_dlg.ia_user, inetd_log_sid());
}

int inetd_attrib_init(HWND hdlg) {
    inetd_attrib_dlg.ia_program = GetDlgItem(hdlg, IDC_MODULE);
    if (!inetd_attrib_dlg.ia_program) {
        return INETD_FALSE;
    }
    SendMessage(inetd_attrib_dlg.ia_program,
                EM_LIMITTEXT,
                (WPARAM)INETD_ATTRIB_MAX,
                (LPARAM)0);
    inetd_attrib_dlg.ia_cmdline = GetDlgItem(hdlg, IDC_CMDLINE);
    if (!inetd_attrib_dlg.ia_cmdline) {
        return INETD_FALSE;
    }
    SendMessage(inetd_attrib_dlg.ia_cmdline,
                EM_LIMITTEXT,
                (WPARAM)INETD_ATTRIB_MAX,
                (LPARAM)0);
    inetd_attrib_dlg.ia_service = GetDlgItem(hdlg, IDC_SERVICE);
    if (!inetd_attrib_dlg.ia_service) {
        return INETD_FALSE;
    }
    SendMessage(inetd_attrib_dlg.ia_service,
                EM_LIMITTEXT,
                (WPARAM)INETD_ATTRIB_MAX,
                (LPARAM)0);
    inetd_attrib_dlg.ia_user = GetDlgItem(hdlg, IDC_USER);
    if (!inetd_attrib_dlg.ia_user) {
        return INETD_FALSE;
    }
    SendMessage(inetd_attrib_dlg.ia_user,
                EM_LIMITTEXT,
                (WPARAM)INETD_ATTRIB_MAX,
                (LPARAM)0);
    inetd_attrib_center(hdlg);
    if (inetd_attrib_dlg.ia_serv) {
        inetd_attrib_display(hdlg, inetd_attrib_dlg.ia_serv);
    }
    return INETD_TRUE;
}

int inetd_attrib_store(HWND hdlg) {
    char text[INETD_ATTRIB_MAX] = "";
    int ivalue;

    if (!inetd_attrib_dlg.ia_serv)
        return INETD_TRUE;
    GetWindowText(inetd_attrib_dlg.ia_service, text, INETD_ATTRIB_MAX-1);
    inetd_attrib_validate(text, "service");
    inetd_conf_set(inetd_attrib_dlg.ia_serv, INETD_SET_SERVICE, text);
    GetWindowText(inetd_attrib_dlg.ia_user, text, INETD_ATTRIB_MAX-1);
    inetd_attrib_validate(text, "user");
    inetd_conf_set(inetd_attrib_dlg.ia_serv, INETD_SET_USER, text);
    GetWindowText(inetd_attrib_dlg.ia_program, text, INETD_ATTRIB_MAX-1);
    inetd_attrib_validate(text, "program");
    inetd_conf_set(inetd_attrib_dlg.ia_serv, INETD_SET_SERVER, text);
    GetWindowText(inetd_attrib_dlg.ia_cmdline, text, INETD_ATTRIB_MAX-1);
    inetd_attrib_validate(text, "command");
    inetd_conf_set(inetd_attrib_dlg.ia_serv, INETD_SET_COMMAND, text);
    if (IsDlgButtonChecked(hdlg, IDC_NORMAL)) {
        ivalue = INETD_NORM_TYPE;
    } else if (IsDlgButtonChecked(hdlg, IDC_TCPMUX)) {
        ivalue = INETD_MUX_TYPE;
    } else {
        ivalue = INETD_MUXPLUS_TYPE;
    }
    inetd_conf_set(inetd_attrib_dlg.ia_serv, INETD_SET_TYPE, &ivalue);
    if (IsDlgButtonChecked(hdlg, IDC_TCP)) {
        strcpy(text, "tcp");
    } else
        strcpy(text, "udp");
    inetd_conf_set(inetd_attrib_dlg.ia_serv, INETD_SET_PROTO, text);
    if (IsDlgButtonChecked(hdlg, IDC_STREAM)) {
        ivalue = SOCK_STREAM;
    } else
        ivalue = SOCK_DGRAM;
    inetd_conf_set(inetd_attrib_dlg.ia_serv, INETD_SET_SOCKTYPE, &ivalue);
    if (IsDlgButtonChecked(hdlg, IDC_NOWAIT)) {
        ivalue = 0;
    } else
        ivalue = 1;
    inetd_conf_set(inetd_attrib_dlg.ia_serv, INETD_SET_WAIT, &ivalue);
    return INETD_TRUE;
}
