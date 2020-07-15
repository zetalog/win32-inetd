#ifndef __INETD_CPL_H
#define __INETD_CPL_H

#include <windows.h>
#include <windowsx.h>
#include <io.h>
#include <commctrl.h>
#include <cpl.h>        /* for control panel applet */
#include <prsht.h>      /* for property sheet */
#include "resource.h"

#define NUM_SUBPROGS    1
#define NUM_SUBSHEETS   2

#define INETD_SERVICE_UNINSTALLED   1
#define INETD_SERVICE_STOPPED       2
#define INETD_SERVICE_RUNNING       3
#define INETD_SERVICE_STARTING      4
#define INETD_SERVICE_STOPPING      5

#define INETD_SERVICE_MAXSTAT       5

#define INETD_STATUS_TIMER          1

#define INETD_ATTRIB_RETURN         1000

typedef struct cpl_sheet {
    int     cp_sheet;   /* dialog box template resource identifier */
    DLGPROC cp_func;    /* dialog box procedure */
} CPLSHEET;

typedef void (CPLFUNC)(HWND, int, int, CPLSHEET*);
typedef struct cpl_prog {
    int         cp_icon;    /* icon resource identifier */
    int         cp_name;    /* name-string resource identifier */
    int         cp_desc;    /* description-string resource identifier */
    int         cp_title;   /* property sheet's title */
    CPLFUNC    *cp_func;    /* property sheet's funciton */
    CPLSHEET   *cp_sheets;  /* property sheet's dialogs */
} CPLPROG;

extern LRESULT CALLBACK inetd_status_proc(HWND hdlg, UINT msg, UINT wparam, LONG lparam);
extern LRESULT CALLBACK inetd_config_proc(HWND hdlg, UINT msg, UINT wparam, LONG lparam);
extern LRESULT CALLBACK inetd_attrib_proc(HWND hdlg, UINT msg, UINT wparam, LONG lparam);

extern void inetd_status_update();

extern void inetd_error_display();
extern void inetd_error_prompt(char *format, ...);

extern void inetd_property_sheet(HWND howner, int icon, int title, CPLSHEET *sheets);

int inetd_service_install(char *module);
int inetd_service_remove();
int inetd_service_start();
int inetd_service_stop();
int inetd_service_query();
char *inetd_service_browse(HWND hparent, char *file, int size);
char *inetd_service_module();

extern HANDLE hinst;
extern SC_HANDLE hmanager;
extern SC_HANDLE hservice;
extern CPLPROG inetd_cpl_prog[NUM_SUBPROGS];

#endif /* __INETD_CPL_H */
