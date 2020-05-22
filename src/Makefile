#CFLAGS += -pg -ggdb -Wall -Wextra -Wpedantic -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wconversion
CC = gcc
CFLAGS += -std=gnu11 -ggdb -D __DEBUG__ 
CFLAGS += -Wall -Wextra -Wpedantic -Wstrict-prototypes -Wmissing-prototypes 
CFLAGS += -Wshadow -Wconversion
CFLAGS += -fsanitize=undefined -fsanitize=leak -fsanitize=address -fpie
LDLIBS =
OBJS = sandbox.o net.o str.o # config.o requests.o sys.o

sandbox: $(OBJS)
	$(CC) $(CFLAGS) $(LDLIBS) $(OBJS) -o $@

sandbox.o:

clean: 
	rm -f $(OBJS) sandbox

gprof: sandbox
	gprof $@ gmon.out > analysis.txt
