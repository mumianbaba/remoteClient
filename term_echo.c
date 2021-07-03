/*
    更NB和方便的操作可以参考 https://blog.csdn.net/zhou_1999/article/details/81173261
    我这就是根据键盘的打印来做的
*/

#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "term_history.h"
#include "term_echo.h"

static AsciiCmd acmd [16];

static void stack_init(TermBuffer* buf, int term_fd){
    if (!buf){
         printf ("stack_init param error\n");
         return;
    }
    buf->size = sizeof(buf->stack[0])/sizeof(char);
    buf->header[0] = 0;
    buf->header[1] = 0;

    buf->tryheader[0] = 0;
    buf->tryheader[1] = 0;

    buf->delta[0] = 0;
    buf->delta[1] = 0;

    buf->acmd.len = 0;
    buf->historyMode = 0;

    buf->term_fd = term_fd;
    return;
}

static int stack_push(int index, TermBuffer* buf, char c){
    if ((index != 0 &&  index != 1)  || !buf){
        printf ("push param error\n");
        return -1;
    }
    char* stack = buf->stack[index];
    int pos = buf->header[index];

    if (pos >= buf->size){
        printf ("stack is full\n");
        return -1;
    }
    stack[pos] = c;

    buf->header[index]++;
    buf->delta[index]++;
    return 0;
}
static int stack_try_pop_Ready(int index, TermBuffer* buf){
    if ((index != 0 &&  index != 1)  || !buf){
        printf ("stack_try_pop_Ready param error\n");
        return -1;
    }
    buf->tryheader[index] = buf->header[index];
    return 0;
}

static int stack_reset_delta(int index, TermBuffer* buf){
    if ((index != 0 &&  index != 1)  || !buf){
        printf ("stack_try_pop_Ready param error\n");
        return -1;
    }
    buf->delta[index] = 0;
    return 0;
}
static int stack_get_delta(int index, TermBuffer* buf){
    if ((index != 0 &&  index != 1)  || !buf){
        printf ("stack_try_pop_Ready param error\n");
        return -1;
    }
    return  buf->delta[index];
}

static int stack_try_pop(int index, TermBuffer* buf, char* c){

    if ((index != 0 &&  index != 1)  || !buf || !c){
        printf ("stack_try_pop param error\n");
        return -1;
    }

    char* stack = buf->stack[index];
    int pos = buf->tryheader[index];
    if (pos <= 0){
         printf ("stack_try_pop stack is empty\n");
         return -1;
    }
    
    *c = stack[pos-1];
    buf->tryheader[index]--;
    return 0;
}


static int stack_pop(int index, TermBuffer* buf, char* c){

    if ((index != 0 &&  index != 1)  || !buf || !c){
        printf ("stack_pop param error\n");
        return -1;
    }

    char* stack = buf->stack[index];
    int pos = buf->header[index];
    if (pos <= 0){
         //printf ("stack_pop stack is empty\n");
         return -1;
    }
    
    *c = stack[pos-1];
    buf->header[index]--;
    buf->delta[index]--;
    return 0;
}

static int stack_size(int index, TermBuffer* buf){

    if ((index != 0 &&  index != 1)  || !buf){
        printf ("size param error\n");
        return -1;
    }
    int pos = buf->header[index];
    return pos;
}
static int stack_dump_for_Send(TermBuffer* buf, char** data, int* len){
    if (!buf || !data || !len){
        printf ("stack_dump_for_Send param error\n");
        return -1;       
    }
    *data = NULL;
    *len = 0;
    int ret = 0;

    int half0  = stack_size (0, buf);
    int half1  = stack_size (1, buf);

    int all  = half0 + half1+1;
    if (all <= 0){
        //printf ("stack_dump_for_Send empty\n");
        return 0;
    }

    *data = calloc(1, all);
    if (!*data){
        printf ("stack_dump_for_Send calloc error\n");
        return -1;
    }

    stack_try_pop_Ready(0, buf);
    char* p = *data + half0-1;
    for ( ; p >= *data ; p--){
        ret = stack_try_pop(0, buf, p);
        if (ret == -1){
            break;
        }
    }

    stack_try_pop_Ready(1, buf);
    p = *data + half0;
    for ( ; p < *data+all-1; p++){
        ret = stack_try_pop(1, buf, p);
        if (ret == -1){
            break;
        }
    }
    *p = '\n';
    *len = all;
    return 0;
}

static int stack_dump_for_print(TermBuffer* buf, char** data, int* len){
    if (!buf || !data || !len){
        printf ("stack_dump_for_print param error\n");
        return -1;       
    }
    int ret = 0;

    *data = NULL;
    *len = 0;
    int half1  = stack_size (1, buf);
    if (half1 <= 0){
        //printf ("stack_dump_for_print empty\n");
        return 0;
    }

    *data = calloc(1, half1);
    if (!*data){
        printf ("stack_dump_for_print calloc error\n");
        return -1;
    }

    stack_try_pop_Ready(1, buf);

    char* p = *data;
    for  ( ; p < *data + half1 ; p++){
        ret = stack_try_pop(1, buf, p);
        if (ret == -1){
            break;
        }
    }
    *len =  half1;
    //printf ("\nstackDumpForPrint ok num:%d\n", half1);
    return 0;
}

static void stack_clear(int index, TermBuffer* buf)
{
    if ((index != 0 &&  index != 1)  || !buf){
        printf ("size param error\n");
        return;
    }
    buf->header[index] = 0;
	buf->delta[index] = 0;
}


static void  ascii_cmd_init()
{
    memset(acmd, 0, sizeof(acmd));
    acmd[CmdUpArrow].len = 4;
    acmd[CmdUpArrow].cmd[0] = 0x1b;
    acmd[CmdUpArrow].cmd[1] = 0x5b;
    acmd[CmdUpArrow].cmd[2] = 0x01;
    acmd[CmdUpArrow].cmd[3] = 0x41;

    acmd[CmdDownArrow].len = 4;
    acmd[CmdDownArrow].cmd[0] = 0x1b;
    acmd[CmdDownArrow].cmd[1] = 0x5b;
    acmd[CmdDownArrow].cmd[2] = 0x01;
    acmd[CmdDownArrow].cmd[3] = 0x42;

    acmd[CmdRightArrow].len = 4;
    acmd[CmdRightArrow].cmd[0] = 0x1b;
    acmd[CmdRightArrow].cmd[1] = 0x5b;
    acmd[CmdRightArrow].cmd[2] = 0x01;
    acmd[CmdRightArrow].cmd[3] = 0x43;

    acmd[CmdLeftArrow].len = 4;
    acmd[CmdLeftArrow].cmd[0] = 0x1b;
    acmd[CmdLeftArrow].cmd[1] = 0x5b;
    acmd[CmdLeftArrow].cmd[2] = 0x01;
    acmd[CmdLeftArrow].cmd[3] = 0x44;

    acmd[CmdDelete].len = 4;
    acmd[CmdDelete].cmd[0] = 0x1b;
    acmd[CmdDelete].cmd[1] = 0x5b;
    acmd[CmdDelete].cmd[2] = 0x33; 
    acmd[CmdDelete].cmd[3] = 0x7e; 
} 

static int ascii_cmd_get(CmdNum number, AsciiCmd* cmd){
    if (number > CmdMax || !cmd){
        return -1;
    }
    memcpy(cmd, &acmd[number], sizeof(AsciiCmd));
    return 0;
}

static void ascii_cmd_feft_move(int fd)
{
    AsciiCmd acmd;
    ascii_cmd_get(CmdLeftArrow,  &acmd);
    acmd.cmd[2] = acmd.cmd[3];
    write(fd, acmd.cmd,3);
}

static void ascii_cmd_right_move(int fd)
{
    AsciiCmd acmd;
    ascii_cmd_get(CmdRightArrow,  &acmd);
    acmd.cmd[2] = acmd.cmd[3];
    write(fd, acmd.cmd,3);
}


static void term_clear_and_print(int fd, TermBuffer* buf, const char* str, int len){

	if (fd < 0 || !buf || !str || len <= 0){
		printf ("term_clear_and_print param error\n");
		return;
	}
	
	char leap;
	int i = 0; 
	int part = stack_size(0, buf) + stack_size(1, buf);
	stack_clear(0,  buf);
	stack_clear(1,  buf);
	for ( i = 0; i < part ;i++){
		ascii_cmd_feft_move(fd);
	}
	leap = ' ';
	for ( i = 0; i < part ;i++){
		write (fd, &leap, 1);
	}
	for ( i = 0; i < part; i++){
		ascii_cmd_feft_move(fd);
	}
	for (i = 0; i < len; i++){
		stack_push(0, buf, str[i]);
	}
	write(fd, str, len);
	return;
}

static int  delete_perspace(char * str, int len){
    int i;
    for (i = 0; i < len ;i++){
        if (str[i] != ' '){
            break;
        }
    }
    if (i == len){
        memset(str, 0, len);
    }
    else if(i > 0){
       memmove(str, str+i, len-i);
    }
    return len-i;
}

static int term_parse_cmd(char* cmd, int len){

    if (!cmd || len <3){
        //printf ("term_handleCmd param error %d\n", len);
        return 0;
    }

    int parsecmd = 0;

    if (cmd[0] != 0x1b){
        //printf ("term_handleCmd [0] not 0x1b\n");
        return -1;
    }

    if (len == 3 && cmd[1] == 0x5b){

        switch (cmd[2])
        {
            case 0x41:{
                /* no print ,wait history cmd */  
                parsecmd = CmdUpArrow;
            }
            break;
            case 0x42:{
                    /* no print ,wait history cmd */
                parsecmd = CmdDownArrow;
            }
            break;
            case 0x43:{
                /*right ->*/
                parsecmd = CmdRightArrow;
            }
            break;
            case 0x44:{
                /*left <- */
                parsecmd = CmdLeftArrow;
            }
            break;
            default:{
               
            }
            break;
        }
    }

    if (len == 4)
    {
        if (cmd[1] == 0x5b && cmd[2] == 0x32 && cmd[3] == 0x7e){
            parsecmd = CmdInsert;
        }
        else if (cmd[1] == 0x5b && cmd[2] == 0x33 && cmd[3] == 0x7e){
            parsecmd = CmdDelete;
        }
        else{
            parsecmd = -1;
        }
    }
    else if (len > 4){
        parsecmd = -1;
    }
    //printf ("cmd:%d\n", parsecmd);
    return parsecmd;
}

int term_open(TermBuffer* ptb){
    if (!ptb){
        printf ("%s:%d param err\n", __func__, __LINE__);
        return -1;
    }
    int term_fd;
	term_fd = open("/dev/tty",O_RDWR|O_NOCTTY);
	if(term_fd < 0){
        term_fd = -1;
        printf ("%s:%d open tty err\n", __func__, __LINE__);
        return -1;
    }
	stack_init(ptb, term_fd);
    ascii_cmd_init();
    return 0;
}

int term_disable_sig(int fd, struct termios* oldTerm){
    struct termios term;
    if (fd < 0){
        return -1;
    }

    tcgetattr(fd, &term);
    if (oldTerm){
        tcgetattr(fd, oldTerm);
    }
    term.c_lflag &= ~(ISIG);
    tcsetattr(fd, TCSANOW, &term);
    return 0;
}


int term_enable_sig(int fd, struct termios* oldTerm){
    struct termios term;
    if (fd < 0){
        return -1;
    }

    tcgetattr(fd, &term);
    if (oldTerm){
        tcgetattr(fd, oldTerm);
    }
    term.c_lflag |= (ISIG);
    tcsetattr(fd, TCSANOW, &term);
    return 0;
}

int term_disable_echo(int fd, struct termios* oldTerm){

    struct termios term;
    if (fd < 0){
        return -1;
    }

    tcgetattr(fd, &term);
    if (oldTerm){
        tcgetattr(fd, oldTerm);
    }
    term.c_lflag &= ~(ICANON|ECHO);
    tcsetattr(fd, TCSANOW, &term);
    return 0;
}

int term_enable_echo(int fd, struct termios* oldTerm){

    struct termios term;
    if (fd < 0){
        return -1;
    }

    tcgetattr(fd, &term);
    if (oldTerm){
        tcgetattr(fd, oldTerm);
    }
    term.c_lflag |= (ICANON|ECHO);
    tcsetattr(fd, TCSANOW, &term);
    return 0;
}

/* return 0, not exit cmd , 1 exit cmd */
int term_self_cmd_exit(char* data, int len){
    if (!data || len <= 0){
        return 0;
    }

    char* str = (char*)calloc (1, len+1);
    if (!str){
        return 0;
    }
    memcpy (str, data, len);
    str[len] = '\0';

    int i = 0;
    char* p = str;
    for (; *p == ' ' || *p == '\t'; p++);

    if (0 != strncmp(p, "telnet", 6)){
        free (str);
        return 0;
    }

    p += 6;
    for (i = 0; *p == ' ' || *p == '\t'; p++, i++);
    if (i == 0){
        free (str);
        return 0;
    }

    if (0 != strncmp(p, "exit", 4)){
        free (str);
        return 0;
    }

    p+=4;
    for (; *p == ' ' || *p == '\t'; p++);

    if (*p != '\0' && *p != '\n'){
        free (str);
        return 0;
    }
    free (str);
    return 1;
}




int term_echo_self(TermBuffer* term_buf, TermHistoryHeader* history, char cc, EchoMode mode , TermLine* line)
{
    if (!term_buf || !line){
        printf("%s:%d param err\n", __func__, __LINE__);
        return -1;
    }
    int ret = 0;
    int action = 0;
    TermBuffer* ptb = term_buf;
    char*cmd = ptb->acmd.cmd;
    int cpos = ptb->acmd.len;
    int historyMode = ptb->historyMode;
    int term_fd = ptb->term_fd;
    int send = 0;
    int print = 0;
    const char* str;
    char leap;
    int i = 0;
	int j = 0;

    line->data = NULL;

    stack_reset_delta(0, ptb);
    stack_reset_delta(1, ptb);

    if (cc == 0x1b){
        cpos = 1;
        cmd[0] = cc;
        ptb->acmd.len = cpos;
        return 0;
    }

    //printf ("--%#x--\n",cc);
    /* no cmd need headler */
    if (cpos <= 0){
        if (cc >=0x20 && cc <= 0x7e){
            stack_push(0, ptb, cc);
            print = 1;
        }
        else if (cc == 0x9){
            cc = 0x20;
            stack_push(0, ptb, cc);
            print = 1;
        }
        else if (cc == 0x7f || cc == 0x8){
            /* <--- del */
            stack_pop(0, ptb, &leap);
            cc = 0;
            print = 1;
        }
        else if (cc == 0x0a){
            //cc = 0;
            //stack_push(0, ptb, cc);
            print = 1;
            send = 1;
        }
    }
    else{
        cmd[cpos++%8] = cc;
        //printf ("\n%#x %#x %#x %d\n", cmd[0], cmd[1], cmd[2], cpos);
        action = term_parse_cmd(cmd, cpos);
        switch (action)
        {
            case CmdUpArrow:
            {
                /* up arrow or down arrow */
                if (!historyMode){
                    historyMode = 1;
                    term_ready_history(history);
                }
                
                int len = 0;
                str = NULL;
                ret = term_get_prev_history(history, &str, &len);
                if (ret == 0 && str && len > 0){
                    term_clear_and_print(term_fd, ptb, str, len);
                }
                cpos = 0;
            }
            break;

            case CmdDownArrow:
            {
                if (historyMode){
                    int len = 0;
                    str = NULL;
                    ret = term_get_next_history(history, &str, &len);
                    if (ret == 0 && history && len > 0){
                        term_clear_and_print(term_fd, ptb, str, len);
                    }
                }   
                cpos = 0;          
            }
            break;

            case CmdRightArrow:
            {
                /* right arrow*/
                if (stack_size(1, ptb) > 0){
                    stack_pop(1, ptb, &leap);
                    //printf ("\n%c\n", leap);
                    stack_push(0, ptb,leap);
                    write(term_fd, cmd, 3);
                }
                cpos = 0; 
            }
            break;

            case CmdLeftArrow:
            {
                /* left arrow*/
                if (stack_size(0, ptb) > 0){
                    stack_pop(0, ptb, &leap);
                    stack_push(1, ptb,leap);
                    //printf ("\n%c\n", leap);
                    write(term_fd, cmd, 3);
                }
                cpos = 0; 
            }
            break;

            case CmdDelete:
            {
                stack_pop(1, ptb, &leap);
                cpos = 0;
                cc = 0;
                print = 1;
            }
            break;
            default:
            {
                if (action != 0){
                    cpos = 0;
                }
            }
            break;
        }
        ptb->acmd.len = cpos;       
    }

    if (print){
        if (mode == SelfEchoDisable && cc > 0){
            cc = '*';
        }

        /* print the current char */
        if (cc > 0){
            write(term_fd, &cc, 1);
        }
        char* p = NULL;
        int len = 0;

        int delat0 = stack_get_delta(0, ptb);
        int delat1 = stack_get_delta(1, ptb);

        //printf ("\ndelat0=%d delat1=%d\n", delat0, delat1);
        for (i = 0; i < -delat0; i++){
            ascii_cmd_feft_move(term_fd);
        }

        ret = stack_dump_for_print(ptb, &p, &len);
        if (ret == 0 && p){
            if (mode == SelfEchoDisable){
                memset(p,'*', len);
            }
            write(term_fd, p, len);
            free (p);
            p = NULL;
        }

        for (i = 0; i < -(delat0 + delat1); i++){
            leap = ' ';
            write(term_fd, &leap, 1);
            len++;
        }
        for( ; len--; ){
            ascii_cmd_feft_move(term_fd);
        }
    }
    if (send){
        /* send with a \n */
        char* p = NULL;
        int len = 0;
        ret = stack_dump_for_Send(ptb, &p, &len);
        if (ret == 0 && p){
            len = delete_perspace(p, len);
            if (len > 0){
                //write(term_fd, p, len);
                line->data = p;
                line->size = len;
                if (len > 1 && mode == SelfEchoEnable){
                    term_add_history(history, p, len-1);
                }
            }
        }
        stack_clear(0, ptb);
        stack_clear(1, ptb);
        historyMode = 0;
    }

    ptb->historyMode = historyMode;
    return 0;
}


#if 0

int main()
{
    int ret = 0;
    int i = 0;
    TermBuffer tb;
    TermHistoryHeader hb;

    ret = term_open(&tb);
    if (ret != 0){
        return -1;
    }
	term_history_init(&hb);

    int  term_fd = tb.term_fd;
    ret = term_disable_echo(term_fd, NULL);
    if (ret != 0){
        printf ("error:term_disable_echo failed\n");
        return -1;
    }
    printf ("debug:echo self\n");

    TermLine line;
    char cc;

    while(1){
        ret = read(term_fd, &cc, 1);
        if (ret != 1){
            continue;
        }
        line.data = NULL;
        ret = term_echo_self(&tb, &hb, cc, SelfEchoEnable, &line);
        if(ret == 0 &&line.data){
            //printf ("need send a data\n");
            //write(term_fd, line.data, line.size);
        }
    }
    printf ("i am while(1)\n");
    while(1);
    return  0;
}
#endif