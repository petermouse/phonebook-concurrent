#ifndef _PHONEBOOK_H
#define _PHONEBOOK_H

#include <pthread.h>
#include <time.h>

#define OPT 1
#define MAX_LAST_NAME_SIZE 16

typedef struct _detail {
    char firstName[16];
    char email[16];
    char phone[10];
    char cell[10];
    char addr1[16];
    char addr2[16];
    char city[16];
    char state[2];
    char zip[5];
} detail;

typedef detail *pdetail;

typedef struct __PHONE_BOOK_ENTRY {
    char *lastName;
    struct __PHONE_BOOK_ENTRY *pNext;
    pdetail dtl;
} entry;

entry *findName(char lastname[], entry *pHead);
entry *task_findName(void *arg);

typedef struct _task_argument {
    char *data_begin;
    char *data_end;
    char lastName[MAX_LAST_NAME_SIZE];
    int numOfTask;
    //int entryCount;
    entry *lEntryPool_begin;    /* The local entry pool */
    entry *lEntry_head; /* local entry linked list */
    entry *lEntry_tail; /* local entry linked list */
} task_arg;

task_arg *createTask_arg(char *data_begin, char *data_end,
                         int numOfTask, entry *entryPool);

void task_append(void *arg);
static double diff_in_second(struct timespec t1, struct timespec t2);

#endif
