#ifndef TELNET_CMD_H
#define TELNET_CMD_H

typedef struct{
    unsigned char cmd[8];
    int  cpos;
}TelnetCmd;

typedef struct{
    TelnetCmd tcmd;
    
}TelnetState;


typedef enum {
    ECHO_ENABLE = 1,
    ECHO_DISABLE,
}TelentCmd;

#define TELNET_IAC          255
#define TELNET_DONT         254
#define TELNET_DO           253
#define TELNET_WONT         252
#define TELNET_WILL         251
#define TELNET_SB           250
#define TELNET_SE           240
#define TELNET_ECHO         1
#define TELNET_SUPGOAHEAD   3


int telnet_iac_parse(TelnetState* pts, unsigned char cc);

#endif



 