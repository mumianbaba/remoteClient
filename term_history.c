#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


#include "term_history.h"

static TermHistoryHeader head;

void term_history_init(TermHistoryHeader* hr){

	if (!hr){
		printf ("term_history_init param error\n");
		return;
	}
	int i = 0;
	hr->cur = NULL;
	hr->size = 0;

	TermHistory* th;
	for ( ; i < MaxHistoryNum; i++) {
		th = (TermHistory*)calloc (1, sizeof(TermHistory));
		th->next = th;
		th->prev = th;
		th->cmd = NULL;
		th->size = 0;
		if (!hr->cur){
			hr->cur = th;
		}
		else{
			th->next = hr->cur->next;
			th->prev = hr->cur;
			hr->cur->next->prev = th;
			hr->cur->next=th;
		}
	}
	return;
}

void term_add_history(TermHistoryHeader* hr, const char* cmd, int len){
	if (!hr || !cmd || len <= 0){
		return;
	}
	
	if (hr->cur->prev->cmd){
		if (len == hr->cur->prev->size &&
		     memcmp(hr->cur->prev->cmd, cmd, len) == 0)
		{
			//printf ("sample cmd, return\n");
			return;
		}
	}
	
	if (hr->cur->cmd){
		free(hr->cur->cmd);
		hr->cur->cmd = NULL;
		hr->cur->size = 0;
	}

	char* p = (char*)calloc (1, len);
	int i;
	for (i = 0; i <len && cmd[i] != '\n'; i++){
		p[i] = cmd[i]; 
	}
	hr->cur->cmd = p;
	hr->cur->size = i;
	p = NULL;
	hr->cur = hr->cur->next;
	return;
}

void term_ready_history(TermHistoryHeader* hr){
	if (!hr){
		printf ("term_ready_history param error\n");
		return ;
	}
	hr->list = hr->cur;
}

int term_get_prev_history(TermHistoryHeader* hr, const char** cmd, int* len){
	if (!hr || !cmd || !len){
		printf ("term_getHistory param error\n");
		return -1;
	}
	*cmd = NULL;
	*len = 0;
	
	TermHistory* prev = hr->list->prev;
	if (!prev->cmd){
		//printf ("term_getHistory cmd null\n");
		return -1;
	}
	
	if (prev == hr->cur){
		//printf ("term_getHistory end of history\n");
		return -1;
	}
	*cmd = prev->cmd;
	*len = prev->size;
	
	hr->list = prev;
	return 0;
} 
int term_get_next_history(TermHistoryHeader* hr, const char** cmd, int* len){
	if (!hr || !cmd || !len){
		printf ("term_getHistory param error\n");
		return -1;
	}
	*cmd = NULL;
	*len = 0;
	
	TermHistory* next = hr->list->next;
	
	if (hr->list == hr->cur || next == hr->cur){
		return -1;
	}
	
	if (!next->cmd){
		//printf ("term_getHistory cmd null\n");
		return -1;
	}
	*cmd = next->cmd;
	*len = next->size;
	
	hr->list = next;
	return 0;
}


void term_print_history(const char* cmd, int len){
	if (!cmd || len <= 0){
		printf ("term_print_history param null\n");
		return;
	}
	
	for ( ; len--; ){
		putchar(*cmd++);
	}
	putchar('\n');
	return;
}


#if 0
int  main (int argc, char** argv)
{
	int ret = 0;
	term_history_init(&head);
	
	char cmd[32];
	int i = 0;
	for ( i = 0; i < 10; i++){
		snprintf(cmd, sizeof(cmd), "cmd string %d", 2);
		term_add_history(&head, cmd, strlen(cmd));
	}
	
	term_ready_history(&head);
	const char* get = NULL;
	int len = 0;
	int j;
	for ( i = 0; i < 20; i++){
		ret = term_getHistory(&head, &get, &len);
		if (ret != 0)
		{
			printf ("term_getHistory failed %d\n", i);
		}
		else{
			term_print_history(get, len);
		}
	}
	
	return 0;
}

#endif