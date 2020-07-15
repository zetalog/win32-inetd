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
 * @(#)chargen.c: familiar character generator
 * $Id: chargen.c,v 1.2 2002/02/07 02:09:00 cvs Exp $
 */

#include <inetd.h>
#include <inetd_int.h>
#include "misc.h"
#include "error.h"
#include <ctype.h>

#define CHARGEN_LINE_SIZ    72
char chargen_ring[128];
char *chargen_ring_end;

void chargen_ring_init() {
    int i;
    
    chargen_ring_end = chargen_ring;
    
    for (i = 0; i <= 128; ++i)
        if (isprint(i))
            *chargen_ring_end++ = i;
}

/* Character generator */
void chargen_stream(SOCKET s, struct inetd_serv *sep) {
    int len;
    char *rs, text[CHARGEN_LINE_SIZ+2];
    
    set_proc_title(sep->is_service, s);
    
    if (!chargen_ring_end) {
        chargen_ring_init();
        rs = chargen_ring;
    }
    
    text[CHARGEN_LINE_SIZ] = '\r';
    text[CHARGEN_LINE_SIZ + 1] = '\n';
    for (rs = chargen_ring; ; ) {
        if ((len = chargen_ring_end - rs) >= CHARGEN_LINE_SIZ)
            memmove(text, rs, CHARGEN_LINE_SIZ);
        else {
            memmove(text, rs, len);
            memmove(text + len, chargen_ring, CHARGEN_LINE_SIZ - len);
        }
        if (++rs == chargen_ring_end)
            rs = chargen_ring;
        if (send(s, text, sizeof (text), 0) != sizeof (text))
            break;
    }
}

/* Character generator */
void chargen_dgram(SOCKET s, struct inetd_serv *sep) {
    struct sockaddr sa;
    static char *rs;
    int len, size;
    char text[CHARGEN_LINE_SIZ+2];
    
    if (chargen_ring_end == 0) {
        chargen_ring_init();
        rs = chargen_ring;
    }
    
    size = sizeof (sa);
    if (inetd_net_recvfrom(s, text, sizeof (text), &sa, &size) < 0)
        return;
    
    if ((len = chargen_ring_end - rs) >= CHARGEN_LINE_SIZ)
        memmove(text, rs, CHARGEN_LINE_SIZ);
    else {
        memmove(text, rs, len);
        memmove(text + len, chargen_ring, CHARGEN_LINE_SIZ - len);
    }
    if (++rs == chargen_ring_end)
        rs = chargen_ring;
    text[CHARGEN_LINE_SIZ] = '\r';
    text[CHARGEN_LINE_SIZ + 1] = '\n';
    (void)sendto(s, text, sizeof (text), 0, &sa, sizeof (sa));
}
