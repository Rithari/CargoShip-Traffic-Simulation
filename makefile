#
# In order to execute this "Makefile" just type "make"
#	A. Delis (ad@di.uoa.gr)
#

OBJS	= master.o common_ipcs.o utils.o nave.o porto.o meteo.o
SOURCE	= master.c common_ipcs.c utils.c nave.c porto.c meteo.c
HEADER	= master.h common_ipcs.h utils.h
OUT	= master nave porto meteo
CC	 = gcc
FLAGS	 = -g3 -c -Wall
LFLAGS	 = -lm
# -g option enables debugging mode
# -c flag generates object code for separate files

vpath %.c source
vpath %.h headers

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)


# create/compile the individual files >>separately<<
master.o: master.c
	$(CC) $(FLAGS) master.c -std=c90

common_ipcs.o: common_ipcs.c
	$(CC) $(FLAGS) common_ipcs.c -std=c90

utils.o: utils.c
	$(CC) $(FLAGS) utils.c -std=c90

nave.o: nave.c
	$(CC) $(FLAGS) nave.c -std=c90

porto.o: porto.c
	$(CC) $(FLAGS) porto.c -std=c90

meteo.o: meteo.c
	$(CC) $(FLAGS) meteo.c -std=c90


# clean house
clean:
	rm -f $(OBJS) $(OUT)

# run the program
run: $(OUT)
	./$(OUT)

# compile program with debugging information
debug: $(OUT)
	valgrind $(OUT)

# run program with valgrind for errors
valgrind: $(OUT)
	valgrind $(OUT)

# run program with valgrind for leak checks
valgrind_leakcheck: $(OUT)
	valgrind --leak-check=full $(OUT)

# run program with valgrind for leak checks (extreme)
valgrind_extreme: $(OUT)
	valgrind --leak-check=full --show-leak-kinds=all --leak-resolution=high --track-origins=yes --vgdb=yes $(OUT)
