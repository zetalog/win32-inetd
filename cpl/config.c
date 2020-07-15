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
 * @(#)config.c: inetd config property sheet
 * $Id: config.c,v 1.12 2002/02/22 05:30:04 zlv Exp $
 */

#include <inetd.h>
#include "inetd_cpl.h"

struct inetd_config {
    HWND            ic_list;    /* service list */
    HWND            is_sheet;   /* property sheet */
    unsigned int    is_max;     /* text maximum size */
    int             is_saved;   /* whether saved changes */
} inetd_config_dlg;

struct inetd_header {
    int             ih_text;
    int             ih_width;
} inetd_config_title[] = {
    { INETD_TITLE_SERVICE,  60 },
    { INETD_TITLE_TYPE,     60 },
    { INETD_TITLE_SOCKTYPE, 60 },
    { INETD_TITLE_PROTOCOL, 50 },
    { INETD_TITLE_FLAGS,    60 },
    { INETD_TITLE_USER,     50 },
    { INETD_TITLE_PROGRAM,  110 },
    { INETD_TITLE_COMMAND,  80 },
};

struct inetd_serv inetd_config_serv;

static int inetd_config_init(HWND hdlg);
static int inetd_config_load();
static int inetd_config_selected();
static int inetd_config_add(HWND hdlg);
static int inetd_config_delete(HWND hdlg);
static int inetd_config_edit(HWND hdlg, int index);
static int inetd_config_get(HWND hdlg, int index, struct inetd_serv *serv);
static int inetd_config_apply(HWND hdlg);

int inetd_config_init(HWND hdlg) {
    LVCOLUMN column;
    int index;
    struct inetd_serv  *serv;

    inetd_config_dlg.is_saved = INETD_TRUE;
    inetd_config_dlg.ic_list = GetDlgItem(hdlg, IDC_SERVICE);
    if (!inetd_config_dlg.ic_list) {
        return INETD_FALSE;
    }
    inetd_config_dlg.is_sheet = GetParent(hdlg);
    if (!inetd_config_dlg.is_sheet) {
        return INETD_FALSE;
    }
    ListView_SetExtendedListViewStyle(inetd_config_dlg.ic_list,
                                      LVS_EX_FULLROWSELECT);
    serv = NULL;
    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    column.fmt = LVCFMT_LEFT;
    for (index = 0; index < 8; index++) {
        TCHAR title[20];
        LoadString(hinst, inetd_config_title[index].ih_text, title, 20);
        column.pszText = title;
        column.cx = inetd_config_title[index].ih_width;
        column.iSubItem = index;
        ListView_InsertColumn(inetd_config_dlg.ic_list, index, &column);
    }
    inetd_config_dlg.is_max = INETD_BUF_SIZE;
	memset(&inetd_config_serv, 0, sizeof (inetd_config_serv));

    return inetd_config_load();
}

void inetd_config_insert(int index, struct inetd_serv *sep) {
    LVITEM lvi = { 0 };
    int i;
    char *text;

    if (!inetd_config_dlg.ic_list || !sep->is_service) {
        return;
    }

    lvi.iItem = index;

    lvi.mask = LVIF_TEXT;
    lvi.pszText = sep->is_service;
    lvi.cchTextMax = sep->is_service?lstrlen(sep->is_service):0;
    if (strlen(lvi.pszText) > inetd_config_dlg.is_max)
        inetd_config_dlg.is_max = strlen(lvi.pszText)+1;
    index = ListView_InsertItem(inetd_config_dlg.ic_list, &lvi);

    for (i = 1; i < 8; i++) {
        switch (i) {
        case 1:
            {
                switch (sep->is_type) {
                case INETD_NORM_TYPE:
                    text = "normal";
                    break;
                case INETD_MUX_TYPE:
                    text = "tcpmux";
                    break;
                case INETD_MUXPLUS_TYPE:
                    text = "muxplus";
                    break;
                default:
                    text = "unknown";
                    break;
                }
            }
            break;
        case 2:
            {
                switch (sep->is_socktype) {
                case SOCK_STREAM:
                    text = "stream";
                    break;
                case SOCK_DGRAM:
                    text = "dgram";
                    break;
                default:
                    text = "unknown";
                    break;
                }
            }
            break;
        case 3:
            text = sep->is_proto;
            break;
        case 4:
            {
                switch (sep->is_wait) {
                case 0:
                    text = "nowait";
                    break;
                case 1:
                    text = "wait";
                    break;
                default:
                    text = "unknown";
                    break;
                }
            }
            break;
        case 5:
            text = sep->is_user;
            break;
        case 6:
            text = sep->is_server;
            break;
        case 7:
            text = sep->is_cmdline;
            break;
        }
		if (text) {
			if (strlen(text) > inetd_config_dlg.is_max)
				inetd_config_dlg.is_max = strlen(text)+1;
		}
        ListView_SetItemText(inetd_config_dlg.ic_list, index, i, text);
    }
    ListView_SetItemState(inetd_config_dlg.ic_list, index,
                          LVIS_FOCUSED | LVIS_SELECTED,
                          LVIS_FOCUSED | LVIS_SELECTED);
}

int inetd_config_load() {
    struct inetd_serv *cp;
    int index;

    if (!inetd_config_dlg.ic_list) {
        return INETD_FALSE;
    }
    if (!inetd_conf_open(INETD_CONF_LOAD)) {
        return INETD_FALSE;
    }
    cp = inetd_conf_load();
    for (index = 0; cp; index++) {
        inetd_config_insert(index, cp);
        inetd_conf_free(cp);
        cp = inetd_conf_load();
    }
    inetd_conf_close();
    return INETD_TRUE;
}

LRESULT CALLBACK inetd_config_proc(HWND hdlg, UINT msg, UINT wparam, LONG lparam) {
    switch (msg) {
        case WM_INITDIALOG:
            return inetd_config_init(hdlg);
        case WM_DESTROY:
            inetd_conf_free(&inetd_config_serv);
            break;
        case WM_COMMAND:
            {
                switch (LOWORD(wparam)) {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hdlg, 1);
                    break;
                case IDC_ADD:
                    if (inetd_config_add(hdlg)) {
						PropSheet_Changed(inetd_config_dlg.is_sheet, hdlg);
                        inetd_config_dlg.is_saved = INETD_FALSE;
                    }
                    break;
                case IDC_REMOVE:
                    if (inetd_config_delete(hdlg)) {
                        PropSheet_Changed(inetd_config_dlg.is_sheet, hdlg);
                        inetd_config_dlg.is_saved = INETD_FALSE;
                    }
                    break;
                }
            }
            break;
        case WM_NOTIFY:
            {
                switch (((NMHDR FAR *) lparam)->code) {
                case NM_DBLCLK:
                    {
                        LV_HITTESTINFO  lvhti;
                        GetCursorPos((LPPOINT)&lvhti.pt);
                        ScreenToClient(((LPNMLISTVIEW)lparam)->hdr.hwndFrom, &lvhti.pt);
                        ListView_HitTest(((LPNMLISTVIEW)lparam)->hdr.hwndFrom, &lvhti);
                        if (lvhti.flags & LVHT_ONITEM) {
                            if (inetd_config_edit(hdlg, lvhti.iItem)) {
                                PropSheet_Changed(inetd_config_dlg.is_sheet, hdlg);
                                inetd_config_dlg.is_saved = INETD_FALSE;
                            }
                        }
                        break;
                    }
                    break;
                case PSN_HELP:
                    {
                        char help_file[MAX_PATH]; /* buffer for name of help file */
                        /* display help for the font properties page. */
                        LoadString(hinst, INETD_HELP, help_file, MAX_PATH);
                        WinHelp(((NMHDR FAR *) lparam)->hwndFrom, help_file,
                                HELP_FINDER, 0);
                    }
                    break;
                case PSN_APPLY:
                    inetd_config_apply(hdlg);
                    break;
                case PSN_KILLACTIVE:
                    if (!inetd_config_dlg.is_saved) {
                        if (IDOK == MessageBox(hdlg, "Save configurations?", "Save", MB_ICONQUESTION | MB_OKCANCEL))
                            inetd_config_apply(hdlg);
                        PropSheet_UnChanged(inetd_config_dlg.is_sheet, hdlg);
                    }
                    break;
                }
                break;
            }
    }
    return 0;
}

int inetd_config_selected() {
    int pos = -1;
    if (!inetd_config_dlg.ic_list)
        return pos;
    pos = ListView_GetNextItem(inetd_config_dlg.ic_list, pos, LVNI_SELECTED);
    return pos;
}

int inetd_config_delete(HWND hdlg) {
    int selected;
    if (!inetd_config_dlg.ic_list)
        return INETD_FALSE;
    selected = inetd_config_selected();
    if (selected >= 0) {
        char    *str, *service;
        LV_ITEM item;
        DWORD   res;
        str = malloc(inetd_config_dlg.is_max+40);
        if (!str)
            return INETD_FALSE;
        item.mask = TVIF_HANDLE | TVIF_TEXT;
        item.iItem = selected;
        item.iSubItem = 0;
        item.pszText = str;
        item.cchTextMax = inetd_config_dlg.is_max;
        ListView_GetItem(inetd_config_dlg.ic_list, &item);
        service = strdup(str);
        sprintf(str, "Confirm to delete service %s ?", service);
        free(service);
        res = MessageBox(hdlg, str, "Confirm", MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO);
        free(str);
        if (res == IDYES) {
            if (ListView_DeleteItem(inetd_config_dlg.ic_list, selected)) {
                ListView_SetItemState(inetd_config_dlg.ic_list, 0,
                                      LVIS_FOCUSED | LVIS_SELECTED,
                                      LVIS_FOCUSED | LVIS_SELECTED);
                return INETD_TRUE;
            }
        }
    }
    return INETD_FALSE;
}

int inetd_config_add(HWND hdlg) {
    DWORD res;
    inetd_conf_free(&inetd_config_serv);
    res = DialogBoxParam(hinst,
						 MAKEINTRESOURCE(INETD_ATTRIB_DLG),
						 hdlg,
						 inetd_attrib_proc,
						 (LPARAM)&inetd_config_serv);
    if (res == INETD_ATTRIB_RETURN) {
        inetd_config_insert(0, &inetd_config_serv);
		return INETD_TRUE;
    }
	return INETD_FALSE;
}

int inetd_config_edit(HWND hdlg, int index) {
    DWORD res;
    struct inetd_serv raw_serv = {0};
    if (!inetd_config_get(hdlg, index, &inetd_config_serv)) {
        MessageBox(hdlg, "Bad selection!", NULL, MB_APPLMODAL | MB_ICONERROR | MB_OK);
        return INETD_FALSE;
    }
    inetd_conf_copy(&raw_serv, &inetd_config_serv);
    res = DialogBoxParam(hinst,
						 MAKEINTRESOURCE(INETD_ATTRIB_DLG),
						 hdlg,
						 inetd_attrib_proc,
						 (LPARAM)&inetd_config_serv);
    if (res == INETD_ATTRIB_RETURN) {
        if (!inetd_conf_compare(&inetd_config_serv, &raw_serv)) {
            inetd_conf_free(&raw_serv);
            return INETD_FALSE;
        }
        ListView_DeleteItem(inetd_config_dlg.ic_list, index);
        inetd_config_insert(index, &inetd_config_serv);
		return INETD_TRUE;
    }
	return INETD_FALSE;
}

/* Retrieve list item from given index, store into the serv */
int inetd_config_get(HWND hdlg, int index, struct inetd_serv *serv) {
    LV_ITEM     item;
    char       *str;
    int         ivalue;
    str = malloc(inetd_config_dlg.is_max);
    if (str) {
        item.mask = TVIF_HANDLE | TVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 0;
        item.pszText = str;
        item.cchTextMax = inetd_config_dlg.is_max;
        if (!ListView_GetItem(inetd_config_dlg.ic_list, &item)) {
            free(str);
            return INETD_FALSE;
        }
        inetd_conf_set(serv, INETD_SET_SERVICE, str);
        item.iSubItem = 1;
        if (!ListView_GetItem(inetd_config_dlg.ic_list, &item)) {
            free(str);
            return INETD_FALSE;
        }
        if (!strcmp(str, "tcpmux"))
            ivalue = INETD_MUX_TYPE;
        else if (!strcmp(str, "muxplus"))
            ivalue = INETD_MUXPLUS_TYPE;
        else
            ivalue = INETD_NORM_TYPE;
        inetd_conf_set(serv, INETD_SET_TYPE, &ivalue);
        item.iSubItem = 2;
        if (!ListView_GetItem(inetd_config_dlg.ic_list, &item)) {
            free(str);
            return INETD_FALSE;
        }
        if (!strcmp(str, "dgram"))
            ivalue = SOCK_DGRAM;
        else
            ivalue = SOCK_STREAM;
        inetd_conf_set(serv, INETD_SET_SOCKTYPE, &ivalue);
        item.iSubItem = 3;
        if (!ListView_GetItem(inetd_config_dlg.ic_list, &item)) {
            free(str);
            return INETD_FALSE;
        }
        inetd_conf_set(serv, INETD_SET_PROTO, str);
        item.iSubItem = 4;
        if (!ListView_GetItem(inetd_config_dlg.ic_list, &item)) {
            free(str);
            return INETD_FALSE;
        }
        if (!strcmp(str, "nowait"))
            ivalue = 0;
        else
            ivalue = 1;
        inetd_conf_set(serv, INETD_SET_WAIT, &ivalue);
        item.iSubItem = 5;
        if (!ListView_GetItem(inetd_config_dlg.ic_list, &item)) {
            free(str);
            return INETD_FALSE;
        }
        inetd_conf_set(serv, INETD_SET_USER, str);
        item.iSubItem = 6;
        if (!ListView_GetItem(inetd_config_dlg.ic_list, &item)) {
            free(str);
            return INETD_FALSE;
        }
        inetd_conf_set(serv, INETD_SET_SERVER, str);
        item.iSubItem = 7;
        if (!ListView_GetItem(inetd_config_dlg.ic_list, &item)) {
            free(str);
            return INETD_FALSE;
        }
        inetd_conf_set(serv, INETD_SET_COMMAND, str);
        free(str);
    }
    return INETD_TRUE;
}

int inetd_config_apply(HWND hdlg) {
    int pos;
    struct inetd_serv serv;

    if (inetd_config_dlg.is_saved)
        return INETD_TRUE;
    if (!inetd_conf_open(INETD_CONF_SAVE)) {
        return INETD_FALSE;
    }
    memset(&serv, 0, sizeof (serv));
    pos = ListView_GetNextItem(inetd_config_dlg.ic_list, -1, LVNI_ALL);
    while (pos != -1) {
        if (inetd_config_get(inetd_config_dlg.ic_list, pos, &serv))
            inetd_conf_save(&serv);
        pos = ListView_GetNextItem(inetd_config_dlg.ic_list, pos, LVNI_ALL);
    }
    inetd_conf_free(&serv);
    inetd_conf_close();
    inetd_config_dlg.is_saved = INETD_TRUE;
    return INETD_TRUE;
}
