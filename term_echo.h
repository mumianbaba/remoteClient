#ifndef TERM_ECHO_H
#define TERM_ECHO_H

#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

typedef enum {
    CmdUpArrow = 1,
    CmdDownArrow = 2,
    CmdRightArrow = 3,
    CmdLeftArrow = 4,
    CmdInsert    = 5,
    CmdDelete    = 6,
    CmdMax       = 15
}CmdNum;


typedef enum{
    SelfEchoDisable = 0,
    SelfEchoEnable = 1,
}EchoMode;

typedef struct{
    char* data;
    int size;
}TermLine;

typedef struct {
    int len;
    char cmd[8];
}AsciiCmd;

typedef struct {
    int size;
    char stack[2][1024];
    int  header[2];
    int  tryheader[2];
    int  delta[2];
    AsciiCmd acmd;
    int historyMode;
    int term_fd;
}TermBuffer;


int term_open(TermBuffer* ptb);
int term_disable_sig(int fd, struct termios* oldTerm);
int term_enable_sig(int fd, struct termios* oldTerm);
int term_disable_echo(int fd, struct termios* oldTerm);
int term_enable_echo(int fd, struct termios* oldTerm);
int term_echo_self(TermBuffer* term_buf, TermHistoryHeader* history, char cc, EchoMode mode , TermLine* line);


int term_self_cmd_exit(char* data, int len);


#endif