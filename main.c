#define  _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>

#include IMPL

/* original version*/
#ifndef OPT
#define OUTPUT_FILE "orig.txt"

/* optimization version*/
#else

#include "text_align.h"
#include "debug.h"
#include <fcntl.h>
#define ALIGN_FILE "align.txt"

#ifndef THREAD_NUM
#define THREAD_NUM 4
#endif

/* thread pool implementation in optimized version*/
#ifdef THREAD_POOL
#include "threadpool.h"

#define OUTPUT_FILE "tp.txt"

#ifndef QUEUE_SIZE
#define QUEUE_SIZE 1024
#endif

#ifndef TASK_NUM
#define TASK_NUM 4
#endif

#else /*original optimized version*/

#define OUTPUT_FILE "opt.txt"

#endif

#endif

#define DICT_FILE "./dictionary/words.txt"

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

int main(int argc, char *argv[])
{
#ifndef OPT
    FILE *fp;
    int i = 0;
    char line[MAX_LAST_NAME_SIZE];
#endif

    struct timespec start, end;
    double cpu_time1, cpu_time2;

    /* File preprocessing */
#ifndef OPT
    /* check file opening */
    fp = fopen(DICT_FILE, "r");
    if (!fp) {
        printf("cannot open the file\n");
        return -1;
    }
#else
    text_align(DICT_FILE, ALIGN_FILE, MAX_LAST_NAME_SIZE);
    int fd = open(ALIGN_FILE, O_RDONLY | O_NONBLOCK);
    off_t file_size = fsize(ALIGN_FILE);
#endif

    /* Build the entry */
    entry *pHead, *e;
    printf("size of entry : %lu bytes\n", sizeof(entry));

#ifndef OPT
    pHead = (entry *) malloc(sizeof(entry));
    e = pHead;
    e->pNext = NULL;
#if defined(__GNUC__)
    __builtin___clear_cache((char *) pHead, (char *) pHead + sizeof(entry));
#endif
    /* Start measuring time*/
    clock_gettime(CLOCK_REALTIME, &start);

    while (fgets(line, sizeof(line), fp)) {
        while (line[i] != '\0')
            i++;
        line[i - 1] = '\0';
        i = 0;
        e = append(line, e);
    }

    /* Stop measuring time*/
    clock_gettime(CLOCK_REALTIME, &end);

    /* close file as soon as possible */
    fclose(fp);
#else
    char *map;
    entry *entry_pool;
    pthread_t threads[THREAD_NUM];

    /* Start measuring time */
    clock_gettime(CLOCK_REALTIME, &start);
    /* Allocate the resource at first */
    map = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    assert(map && "mmap error");
    entry_pool = (entry *)malloc(sizeof(entry) *
                                 file_size / MAX_LAST_NAME_SIZE);
    assert(entry_pool && "entry_pool error");

    /* Prepare for multi-threading */
    pthread_setconcurrency(THREAD_NUM + 1);
#ifndef THREAD_POOL
    thread_arg *thread_args[THREAD_NUM];
    for (int i = 0; i < THREAD_NUM; i++)
        // Created by malloc, remember to free them.
        thread_args[i] = createThread_arg(map + MAX_LAST_NAME_SIZE * i, map + file_size, i,
                                          THREAD_NUM, entry_pool + i);
    /* Deliver the jobs to all threads and wait for completing */
    for (int i = 0; i < THREAD_NUM; i++)
        pthread_create(&threads[i], NULL, (void *)&append, (void *)thread_args[i]);

    for (int i = 0; i < THREAD_NUM; i++)
        pthread_join(threads[i], NULL);
#else
    task_arg *task_args[TASK_NUM];
    for (int i = 0; i < TASK_NUM; i++)
        task_args[i] = createTask_arg(map + MAX_LAST_NAME_SIZE * i,
                                      map + file_size, TASK_NUM, entry_pool + i);

    /* thread pool implementation in optimized version*/
    DEBUG_LOG("Use thread pool\n");
    threadpool_t *tp = threadpool_create(THREAD_NUM, QUEUE_SIZE, 0);
    assert(tp && "thread pool error");
    for (int i = 0; i < TASK_NUM; i++)
        threadpool_add(tp, (void *)&task_append, (void *)task_args[i], 0);
    threadpool_destroy(tp, 1); //graceful terminate
#endif

#ifndef THREAD_POOL
    /* Connect the linked list of each thread */
    for (int i = 0; i < THREAD_NUM; i++) {
        if (i == 0) {
            pHead = thread_args[i]->lEntry_head;
            DEBUG_LOG("Connect %d head string %s %p\n", i,
                      pHead->lastName, thread_args[i]->data_begin);
        } else {
            e->pNext = thread_args[i]->lEntry_head;
            DEBUG_LOG("Connect %d head string %s %p\n", i,
                      e->pNext->lastName, thread_args[i]->data_begin);
        }

        e = thread_args[i]->lEntry_tail;
        DEBUG_LOG("Connect %d tail string %s %p\n", i,
                  e->lastName, thread_args[i]->data_begin);
        DEBUG_LOG("round %d\n", i);
    }
#endif
#endif
    /* Stop measuring time*/
    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time1 = diff_in_second(start, end);

    /* Find the given entry */
    /* the givn last name to find */
    char input[MAX_LAST_NAME_SIZE] = "zyxel";

#ifndef THREAD_POOL
    e = pHead;
    assert(findName(input, e) &&
           "Did you implement findName() in " IMPL "?");
    assert(0 == strcmp(findName(input, e)->lastName, "zyxel"));

#if defined(__GNUC__)
    __builtin___clear_cache((char *) pHead, (char *) pHead + sizeof(entry));
#endif

#endif
    /* Compute the execution time */
    clock_gettime(CLOCK_REALTIME, &start);
#ifndef THREAD_POOL
    findName(input, e);
#else
    tp = threadpool_create(THREAD_NUM, QUEUE_SIZE, 0);
    assert(tp && "thread pool error");
    for (int i = 0; i < TASK_NUM; i++) {
        strcpy(task_args[i]->lastName, input);
        threadpool_add(tp, (void *)&task_findName, (void *)task_args[i], 0);
    }
    threadpool_destroy(tp, 1); //graceful terminate
#endif
    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time2 = diff_in_second(start, end);

    /* Write the execution time to file. */
    FILE *output;
    output = fopen(OUTPUT_FILE, "a");
    fprintf(output, "append() findName() %lf %lf\n", cpu_time1, cpu_time2);
    fclose(output);

    printf("execution time of append() : %lf sec\n", cpu_time1);
    printf("execution time of findName() : %lf sec\n", cpu_time2);

    /* Release memory */
#ifndef OPT
    while (pHead) {
        e = pHead;
        pHead = pHead->pNext;
        free(e);
    }
#else

#ifndef THREAD_POOL
    /* Free the allocated detail entry */
    e = pHead;
    while (e) {
        free(e->dtl);
        e = e->pNext;
    }
#else
    for (int i = 0; i < TASK_NUM; ++i) {
        e = task_args[i]->lEntry_head;
        while(e) {
            free(e->dtl);
            e = e->pNext;
        }
    }
#endif
    free(entry_pool);
#ifndef THREAD_POOL
    for (int i = 0; i < THREAD_NUM; ++i)
        free(thread_args[i]);
#else
    for (int i = 0; i < TASK_NUM; ++i)
        free(task_args[i]);
#endif
    munmap(map, file_size);
    close(fd);
#endif
    return 0;
}
