CC ?= gcc
CFLAGS_common ?= -Wall -std=gnu99
CFLAGS_orig = -O0
CFLAGS_opt  = -O0 -pthread -g
CFLAGS_tp = -O0 -pthread -g

ifdef CHECK_LEAK
CFLAGS_common += -fsanitize=address -fno-omit-frame-pointer
endif

ifdef THREAD
CFLAGS_opt  += -D THREAD_NUM=${THREAD}
endif

ifdef QUEUE
CFLAGS_tp += -D QUEUE_SIZE=${QUEUE}
endif

ifdef TASK
CFLAGS_tp += -D TASK_NUM=${TASK}
endif

ifeq ($(strip $(PROFILE)),1)
CFLAGS_opt += -pg
CFLAGS_tp += -pg
endif

ifeq ($(strip $(DEBUG)),1)
CFLAGS_opt += -DDEBUG
CFLAGS_tp += -DDEBUG
endif

EXEC = phonebook_orig phonebook_opt phonebook_tp
GIT_HOOKS := .git/hooks/applied
.PHONY: all
all: $(GIT_HOOKS) $(EXEC)

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

SRCS_common = main.c

tools/text_align: text_align.c tools/tool-text_align.c
	$(CC) $(CFLAGS_common) $^ -o $@

phonebook_orig: $(SRCS_common) phonebook_orig.c phonebook_orig.h
	$(CC) $(CFLAGS_common) $(CFLAGS_orig) \
		-DIMPL="\"$@.h\"" -o $@ \
		$(SRCS_common) $@.c

phonebook_opt: $(SRCS_common) phonebook_opt.c phonebook_opt.h text_align.c
	$(CC) $(CFLAGS_common) $(CFLAGS_opt) \
		-DIMPL="\"$@.h\"" -o $@ \
		$(SRCS_common) $@.c text_align.c

phonebook_tp: $(SRCS_common) phonebook_tp.c phonebook_tp.h text_align.c threadpool.h threadpool.c
	$(CC) $(CFLAGS_common) $(CFLAGS_tp) \
		-DIMPL="\"$@.h\"" -DTHREAD_POOL -o $@ \
		$(SRCS_common) $@.c text_align.c threadpool.c

run: $(EXEC)
	echo 3 | sudo tee /proc/sys/vm/drop_caches
	watch -d -t "./phonebook_orig && echo 3 | sudo tee /proc/sys/vm/drop_caches"

cache-test: $(EXEC)
	perf stat --repeat 100 \
		-e cache-misses,cache-references,instructions,cycles \
		./phonebook_orig
	perf stat --repeat 100 \
		-e cache-misses,cache-references,instructions,cycles \
		./phonebook_opt
	perf stat --repeat 100 \
		-e cache-misses,cache-references,instructions,cycles \
		./phonebook_tp

output.txt: cache-test calculate
	./calculate

plot: output.txt
	gnuplot scripts/runtime.gp

calculate: calculate.c
	$(CC) $(CFLAGS_common) $^ -o $@

.PHONY: clean
clean:
	$(RM) $(EXEC) *.o perf.* *.txt\
		calculate runtime.png gmon.out
