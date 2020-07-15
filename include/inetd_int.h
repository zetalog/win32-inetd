#ifndef __INETD_INTERNAL_H
#define __INETD_INTERNAL_H

/* built in services */
void chargen_dgram(SOCKET, struct inetd_serv *);
void chargen_stream(SOCKET, struct inetd_serv *);
void daytime_dgram(SOCKET, struct inetd_serv *);
void daytime_stream(SOCKET, struct inetd_serv *);
void machtime_dgram(SOCKET, struct inetd_serv *);
void machtime_stream(SOCKET, struct inetd_serv *);
void discard_dgram(SOCKET, struct inetd_serv *);
void discard_stream(SOCKET, struct inetd_serv *);
void echo_dgram(SOCKET, struct inetd_serv *);
void echo_stream(SOCKET, struct inetd_serv *);
struct inetd_serv *tcpmux(SOCKET);

extern struct inetd_serv *inetd_serv_tab;
extern int inetd_sess_end;
extern struct inetd_builtin  inetd_builtin_services[];
extern SOCKET inetd_sock_max;
extern CRITICAL_SECTION inetd_mutex_env;

#endif /* __INETD_INTERNAL_H */
