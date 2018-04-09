CFLAGS += -Wall -Werror -Wextra -pedantic
CFLAGS += -Wmissing-prototypes
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wmissing-declarations

# Uncomment to demo c99 parametric testing.
#CFLAGS += -std=c99

# Uncomment to disable setjmp()/longjmp().
#CFLAGS += -DGREATEST_USE_LONGJMP=0

# Uncomment to disable clock() / time.h.
#CFLAGS += -DGREATEST_USE_TIME=0

all: example

example: example.c greatest.h example-suite.o
	${CC} -o $@ example.c example-suite.o ${CFLAGS} ${LDFLAGS}

clean:
	rm -f example *.o *.core

example-suite.o: example-suite.c
