CFLAGS = -D_GNU_SOURCE -g3 -Wall -Wextra -Wconversion -Wcast-qual -Wcast-align -g
CFLAGS += -Winline -Wfloat-equal -Wnested-externs
CFLAGS += -pedantic -std=gnu99 -Werror

PROMPT = -DPROMPT

EXECS = 33sh 33noprompt


.PHONY: clean all

all: $(EXECS)

33sh: sh.c jobs.c
	$(CC) $(CFLAGS) $(PROMPT) $^ -o $@

33noprompt: sh.c jobs.c
	$(CC) $(CFLAGS) $^ -o $@
clean:
	rm -f $(EXECS)
