#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "telnet_cmd.h"


int telnet_iac_parse(TelnetState* pts, unsigned char cc){

    if (!pts){
        printf ("%s:%d param error\n", __func__, __LINE__);
        return -1;
    }
    unsigned char* cmd = &pts->tcmd.cmd[0];
    int cpos = pts->tcmd.cpos;
    if (cc == 0xff){
        cmd[0] = cc;
        cpos = 1;
        pts->tcmd.cpos = cpos;
        return 0;
    }

    if (cpos > 0){
        cmd[cpos++%8] = cc;
    }

    if (cpos == 3){
        switch(cmd[1]){
            case TELNET_DO:
            {
                if (cmd[2] == TELNET_ECHO){
                    return ECHO_ENABLE;
                }
            }
            break;

            case TELNET_WILL:
            {

            }
            break;
            
            case TELNET_WONT:
            {

            }
            break;

            case TELNET_DONT:
            {
                if (cmd[2] == TELNET_ECHO){
                    return ECHO_DISABLE;
                }
                printf ("zzz...\n");
            }
            break;
            default:
                printf ("unkown cmd\n");
            break;
        }
    }
    
    if (cpos >= 3){
        cpos = 0;
    }

    pts->tcmd.cpos = cpos;
    return 0;
}



int handler_do(int opt){



}

