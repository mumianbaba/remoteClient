#ifndef TERM_HISTORY_H
#define TERM_HISTORY_H

#define MaxHistoryNum  20

struct _TermHistory{
	struct _TermHistory* next;
	struct _TermHistory* prev;
	char* cmd;
	int size;
};


typedef struct _TermHistory TermHistory;


typedef struct{
	TermHistory*  cur;
	TermHistory*  list;
	int size;
}TermHistoryHeader;



void term_history_init(TermHistoryHeader* hr);

void term_add_history(TermHistoryHeader* hr, const char* cmd, int len);

int term_get_prev_history(TermHistoryHeader* hr, const char** cmd, int* len);

int term_get_next_history(TermHistoryHeader* hr, const char** cmd, int* len);

void term_ready_history(TermHistoryHeader* hr);

#endif
