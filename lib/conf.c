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
 * @(#)conf.c: inetd configuration routines
 * $Id: conf.c,v 1.7 2002/02/07 08:44:01 cvs Exp $
 */

#include <inetd.h>
#include <inetd_int.h>
#include "misc.h"

/* Configuration file's header */
#define INETD_CONF_HEADER       "\
# Copyright (c) 1999-2002 Prospect Corp.\n\
#\n\
# inetd.conf\tThis file describes the services that will be available\n\
#\t\tthrough the INETD TCP/IP super server.  To re-configure\n\
#\t\tthe running INETD process, edit this file, then restart\n\
#\t\tthe INETD service process.\n\
#\n\
# Version:\t@(#)/etc/inetd.conf	3.10	05/27/93\n\
#\n\
# Authors:\tOriginal taken from BSD UNIX 4.3/TAHOE.\n\
#\t\tModified for Win32 by Lv \"Zetalog\" Zheng <zl@prospect.com.cn>\n\
#\n\
# <service_name> <sock_type> <proto> <flags> <user> <server_path> <args>\n\n"

FILE *inetd_conf_file = NULL;
char line[INETD_LINE_MAX];
static struct inetd_serv serv;

static char inetd_conf_path[] = "\\drivers\\etc\\inetd.conf";

char *sskip(char **cpp);
char *skip(char **cpp);
char *nextline(FILE *fd);

/* Safe skip - if skip returns null, log a syntax error in the
 * configuration file and exit.
 */
char *sskip(char **cpp) {
    char *cp;
    cp = skip(cpp);
    if (cp == NULL) {
        inetd_log_message(INETD_LOG_FAILURE, "safe skip failure.");
        exit(-1);
    }
    return (cp);
}

/* Skip spaces.
 */
char *skip(char **cpp) {
    char *cp = *cpp;
    char *start;
again:
    while (*cp == ' ' || *cp == '\t')
        cp++;
    if (*cp == '\0') {
        int c;
        c = getc(inetd_conf_file);
        (void)ungetc(c, inetd_conf_file);
        if (c == ' ' || c == '\t') {
            if (cp = nextline(inetd_conf_file))
                goto again;
        }
        *cpp = (char *)0;
        return ((char *)0);
    }
    start = cp;
    while (*cp && *cp != ' ' && *cp != '\t')
        cp++;
    if (*cp != '\0')
        *cp++ = '\0';
    *cpp = cp;
    return (start);
}

/* Read next line from a file descriptor.
 */
char *nextline(FILE *fd) {
    char *cp;
    if (fgets(line, sizeof (line), fd) == NULL)
        return ((char *)0);
    cp = strchr(line, '\n');
    if (cp)
        *cp = '\0';
    return (line);
}

/* Set config file
 * Returns: int value, 0 on failure while other on success
 */
int inetd_conf_open(int how) {
    char *CONFIG;
    int len, size;
    CONFIG = NULL;
    if (inetd_conf_file != NULL) {
        inetd_conf_close();
    }
    len = GetSystemDirectory(CONFIG, 0);
    len += strlen(inetd_conf_path);
    CONFIG = (char *)memset((void *)fsget(len), 0, len);
    size = GetSystemDirectory(CONFIG, len-1);
    CONFIG[size] = '\0';
    strcat(CONFIG, inetd_conf_path);
    inetd_conf_file = fopen(CONFIG,
                            (how == INETD_CONF_LOAD) ? "r" : "w");
    fsgive((void **)&CONFIG);
    if (how == INETD_CONF_SAVE && inetd_conf_file) {
        fwrite(INETD_CONF_HEADER, 1, strlen(INETD_CONF_HEADER), inetd_conf_file);
    }
    return (inetd_conf_file != NULL);
}

/* Close config file
 */
void inetd_conf_close() {
    if (inetd_conf_file) {
        (void)fclose(inetd_conf_file);
        inetd_conf_file = NULL;
    }
}

/* Get a service from config file
 * Returns: * pointer to service table
 */
struct inetd_serv *inetd_conf_load() {
    struct inetd_serv *sep = &serv;
    char *cp, *arg;
    static char TCPMUX_TOKEN[] = "tcpmux/";
#define TCP_MUX_LEN     (sizeof (TCPMUX_TOKEN)-1)

more:
    while ((cp = nextline(inetd_conf_file)) && (*cp == '#' || *cp == '\0'));
    if (cp == NULL)
        return ((struct inetd_serv *)0);
    /* clear the static buffer, since some fields (is_ctrladdr,
     * for example) don't get initialized here.
     */
    memset(sep, 0, sizeof (*sep));
    arg = skip(&cp);
    if (cp == NULL) {
        /* got an empty line containing just blanks/tabs. */
        goto more;
    }
    if (strncmp(arg, TCPMUX_TOKEN, TCP_MUX_LEN) == 0) {
        char *c = arg + TCP_MUX_LEN;
        if (*c == '+') {
            sep->is_type = INETD_MUXPLUS_TYPE;
            c++;
        } else
            sep->is_type = INETD_MUX_TYPE;
        sep->is_service = newstr(c);
    } else {
        sep->is_service = newstr(arg);
        sep->is_type = INETD_NORM_TYPE;
    }
    arg = sskip(&cp);
    if (strcmp(arg, "stream") == 0)
        sep->is_socktype = SOCK_STREAM;
    else if (strcmp(arg, "dgram") == 0)
        sep->is_socktype = SOCK_DGRAM;
    else if (strcmp(arg, "raw") == 0)
        sep->is_socktype = SOCK_RAW;
    else
        sep->is_socktype = -1;
    sep->is_proto = newstr(sskip(&cp));
    arg = sskip(&cp);
    sep->is_wait = strcmp(arg, "wait") == 0;    /* no use */
    if (INETD_ISMUX(sep)) {
        /*
         * Silently enforce "nowait" for TCPMUX services since
         * they don't have an assigned port to listen on.
         */
        sep->is_wait = 0;

        if (strcmp(sep->is_proto, "tcp")) {
            inetd_log_message(INETD_LOG_ERROR, 
                              "%s: bad protocol for tcpmux service %s",
                              "inetd", sep->is_service);
            goto more;
        }
        if (sep->is_socktype != SOCK_STREAM) {
            inetd_log_message(INETD_LOG_ERROR,
                              "%s: bad socket type for tcpmux service %s",
                              "inetd", sep->is_service);
            goto more;
        }
    }
    sep->is_user = newstr(sskip(&cp));
    sep->is_server = newstr(sskip(&cp));
    if (strcmp(sep->is_server, "internal") == 0) {
        struct inetd_builtin *bi;
        
        for (bi = inetd_builtin_services; bi->ib_service; bi++)
            if (bi->ib_socktype == sep->is_socktype &&
                strcmp(bi->ib_service, sep->is_service) == 0)
                break;
            if (bi->ib_service == 0) {
                inetd_log_message(INETD_LOG_ERROR, "internal service %s unknown",
                                  sep->is_service);
                goto more;
            }
            sep->is_builtin = bi;
            sep->is_wait = bi->ib_wait;
    } else
        sep->is_builtin = NULL;
    sep->is_cmdline = newstr(sskip(&cp));
    return (sep);
}

int inetd_conf_save(struct inetd_serv *cp) {
    if (!inetd_conf_file || !cp) {
        return INETD_FALSE;
    }
    if (!cp->is_service || !cp->is_user || !cp->is_proto ||
        !cp->is_server || !cp->is_cmdline)
        return INETD_FALSE;
    switch (cp->is_type) {
    case INETD_NORM_TYPE:
        break;
    case INETD_MUX_TYPE:
        fprintf(inetd_conf_file, "%s", "tcpmux/");
        break;
    case INETD_MUXPLUS_TYPE:
        fprintf(inetd_conf_file, "%s", "tcpmux/+");
        break;
    }
    fprintf(inetd_conf_file,
            "%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
            cp->is_service,
            cp->is_socktype != SOCK_DGRAM ? "stream" : "dgram",
            cp->is_proto,
            cp->is_wait ? "wait" : "nowait",
            cp->is_user,
            cp->is_server,
            cp->is_cmdline);
    return INETD_TRUE;
}

/* Free a service table
 * Accepts: * pointer to service table
 */
void inetd_conf_free(struct inetd_serv *cp) {
    if (cp->is_service)
        fsgive((void **)&(cp->is_service));
    if (cp->is_proto)
        fsgive((void **)&(cp->is_proto));
    if (cp->is_user)
        fsgive((void **)&(cp->is_user));
    if (cp->is_server)
        fsgive((void **)&(cp->is_server));
    if (cp->is_cmdline)
        fsgive((void **)&(cp->is_cmdline));
}

/* Store a service table to the global list
 * Accepts: * pointer to service table
 * Returns: * pointer to service table
 */
struct inetd_serv *inetd_conf_alloc(struct inetd_serv *cp) {
    struct inetd_serv *sep;
    sep = (struct inetd_serv *)fsget(sizeof (*sep));
    if (sep == (struct inetd_serv *)0) {
        inetd_log_message(INETD_LOG_FAILURE, "inetd_conf_get: malloc failure.");
        exit(-1);
    }
    *sep = *cp;
    sep->is_fd = INVALID_SOCKET;
    sep->is_next = inetd_serv_tab;
    inetd_serv_tab = sep;
    return (sep);
}

/* Set configuration values
 * Accepts: * pointer to service table
 *          what to be set
 *          * pointer to the value
 */
void inetd_conf_set(struct inetd_serv *cp, int what, void *value) {
    if (!cp)
        return;
    switch (what) {
    case INETD_SET_SERVICE:
        if (cp->is_service)
            free(cp->is_service);
        cp->is_service = newstr((char *)value);
        break;
    case INETD_SET_USER:
        if (cp->is_user)
            free(cp->is_user);
        cp->is_user = newstr((char *)value);
        break;
    case INETD_SET_SERVER:
        if (cp->is_server)
            free(cp->is_server);
        cp->is_server = newstr((char *)value);
        break;
    case INETD_SET_COMMAND:
        if (cp->is_cmdline)
            free(cp->is_cmdline);
        cp->is_cmdline = newstr((char *)value);
        break;
    case INETD_SET_PROTO:
        if (cp->is_proto)
            free(cp->is_proto);
        cp->is_proto = newstr((char *)value);
        break;
    case INETD_SET_WAIT:
        cp->is_wait = *(int *)(value);
        break;
    case INETD_SET_SOCKTYPE:
        cp->is_socktype = *(int *)(value);
        break;
    case INETD_SET_TYPE:
        cp->is_type = *(int *)value;
        break;
    default:
        break;
    }
}

void inetd_conf_copy(struct inetd_serv *cp, struct inetd_serv *value) {
    if (!cp || !value)
        return;
    if (cp->is_service)
        free(cp->is_service);
    cp->is_service = value->is_service ? newstr((char *)value->is_service) : 0;
    if (cp->is_user)
        free(cp->is_user);
    cp->is_user = value->is_user ? newstr((char *)value->is_user) : 0;
    if (cp->is_server)
        free(cp->is_server);
    cp->is_server = value->is_server ? newstr((char *)value->is_server) : 0;
    if (cp->is_proto)
        free(cp->is_proto);
    cp->is_proto = value->is_proto ? newstr((char *)value->is_proto) : 0;
    if (cp->is_cmdline)
        free(cp->is_cmdline);
    cp->is_cmdline = value->is_cmdline ? newstr((char *)value->is_cmdline) : 0;
    cp->is_wait = value->is_wait;
    cp->is_socktype = value->is_socktype;
    cp->is_type = value->is_type;
}

int inetd_conf_compare(struct inetd_serv *cp, struct inetd_serv *value) {
    int ret;

    if (!cp || !value)
        return 0;
#define COMPARE_VALUE(x, y)         \
    do {                            \
        if (!x && !y) ;             \
        else {                      \
            if (!x || !y) return 1; \
            ret = strcmp(x, y);     \
            if (ret) return ret;    \
        }                           \
    } while (0)
    COMPARE_VALUE(cp->is_service, value->is_service);
    COMPARE_VALUE(cp->is_user, value->is_user);
    COMPARE_VALUE(cp->is_server, value->is_server);
    COMPARE_VALUE(cp->is_proto, value->is_proto);
    COMPARE_VALUE(cp->is_cmdline, value->is_cmdline);
#undef COMPARE_VALUE
#define COMPARE_VALUE(x, y)         \
    do {                            \
        if (x != y) return y - x;   \
    } while (0)
    COMPARE_VALUE(cp->is_wait, value->is_wait);
    COMPARE_VALUE(cp->is_socktype, value->is_socktype);
    COMPARE_VALUE(cp->is_type, value->is_type);
    return 0;
}
