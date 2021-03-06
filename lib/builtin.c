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
 * @(#)builtin.c: inetd built in services
 * $Id: builtin.c,v 1.5 2002/02/20 07:53:45 zlv Exp $
 */

#include <inetd.h>
#include <inetd_int.h>

/* Add built in services to this vector */
struct inetd_builtin  inetd_builtin_services[] = {
    /* Echo received data */
    { "echo",       SOCK_STREAM,    0,  echo_stream },
    { "echo",       SOCK_DGRAM,     0,  echo_dgram },
    
    /* Internet /dev/null */
    { "discard",    SOCK_STREAM,    0,  discard_stream },
    { "discard",    SOCK_DGRAM,     0,  discard_dgram },
    
    /* Return 32 bit time since 1970 */
    { "time",       SOCK_STREAM,    0,  machtime_stream },
    { "time",       SOCK_DGRAM,     0,  machtime_dgram },
    
    /* Return human-readable time */
    { "daytime",    SOCK_STREAM,    0,  daytime_stream },
    { "daytime",    SOCK_DGRAM,     0,  daytime_dgram },
    
    /* Familiar character generator */
    { "chargen",    SOCK_STREAM,    0,  chargen_stream },
    { "chargen",    SOCK_DGRAM,     0,  chargen_dgram },
    
    /* TCP port mutiplexter */
    { "tcpmux",     SOCK_STREAM,    0,  (void (*)())tcpmux },
    
    { NULL }
};
