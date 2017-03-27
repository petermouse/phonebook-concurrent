#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "phonebook_tp.h"
#include "debug.h"

entry *findName(char lastname[], entry *pHead)
{
    size_t len = strlen(lastname);
    while (pHead) {
        if (strncasecmp(lastname, pHead->lastName, len) == 0
                && (pHead->lastName[len] == '\n' ||
                    pHead->lastName[len] == '\0')) {
            pHead->lastName[len] = '\0';
            if (!pHead->dtl)
                pHead->dtl = (pdetail) malloc(sizeof(detail));
            return pHead;
        }
        //DEBUG_LOG("find string = %s\n", pHead->lastName);
        pHead = pHead->pNext;
    }
    return NULL;
}

entry *task_findName(void *arg)
{
    task_arg *t_arg = (task_arg *)arg;
    entry* e = t_arg->lEntry_head;
    size_t len = strlen(t_arg->lastName);
    while (e) {
        if (strncasecmp(t_arg->lastName, e->lastName, len) == 0
                && (e->lastName[len] == '\n' ||
                    e->lastName[len] == '\0')) {
            e->lastName[len] = '\0';
            //DEBUG_LOG("find string = %s\n", e->lastName);
            if (!e->dtl)
                e->dtl = (pdetail) malloc(sizeof(detail));
            DEBUG_LOG("Find the last name !\n");
            return e;
        }
        e = e->pNext;
    }
    return NULL;
}

task_arg *createTask_arg(char *data_begin, char *data_end,
                         int numOfTask, entry *entryPool)
{
    task_arg *new_arg = (task_arg *) malloc(sizeof(task_arg));

    new_arg->data_begin = data_begin;
    new_arg->data_end = data_end;
    new_arg->numOfTask = numOfTask;
    new_arg->lEntryPool_begin = new_arg->lEntry_head = new_arg->lEntry_tail = entryPool;
    return new_arg;
}

/**
 * Generate a local linked list in task.
 */
void task_append(void *arg)
{
    struct timespec start, end;
    double cpu_time;

    clock_gettime(CLOCK_REALTIME, &start);

    task_arg *t_arg = (task_arg *) arg;

    int count = 0;
    entry *j = t_arg->lEntryPool_begin;
    for (char *i = t_arg->data_begin; i < t_arg->data_end;
            i += MAX_LAST_NAME_SIZE * t_arg->numOfTask,
            j += t_arg->numOfTask, count++) {
        /* Append the new at the end of the local linked list */
        t_arg->lEntry_tail->pNext = j;
        t_arg->lEntry_tail = t_arg->lEntry_tail->pNext;
        t_arg->lEntry_tail->lastName = i;
        t_arg->lEntry_tail->pNext = NULL;
        t_arg->lEntry_tail->dtl = NULL;
    }
    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time = diff_in_second(start, end);

    DEBUG_LOG("task take %lf sec, count %d\n", cpu_time, count);
}

void show_entry(entry *pHead)
{
    while (pHead) {
        printf("%s", pHead->lastName);
        pHead = pHead->pNext;
    }
}

static double diff_in_second(struct timespec t1, struct timespec t2)
{
    struct timespec diff;
    if (t2.tv_nsec-t1.tv_nsec < 0) {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec - 1;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
    } else {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }
    return (diff.tv_sec + diff.tv_nsec / 1000000000.0);
}
