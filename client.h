#ifndef CLIENT_H
#define CLIENT_H

#define TELPROXY_SYN            "remote-login"
#define TELPROXY_OK             "telnet-ok"
#define TELPROXY_EXIT           "telnet exit"

#define DEFAULT_DEVID           "Tenbay-001"
#define DEFAULT_SERVER_IP       "192.168.99.117"
#define DEFAULT_PORT            50000
#define DEFAULT_MAC             "11:22:33:44:55:66"

#define TELPROXY_TYPE_DEVICE  1
#define TELPROXY_TYPE_CLINET  2


 struct TelProxyLogin{
     char sync[12];
     unsigned int  len;
     unsigned int type;
     char id[64];
 } __attribute__ ((packed));


#define dbg_ptf(dbg, format, ...)   if (dbg >0)printf("%s:%d"format,__func__, __LINE__, ##__VA_ARGS__)

#endif
