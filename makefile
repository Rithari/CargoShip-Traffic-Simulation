#
# Compiler flags
#
CC     = gcc
CFLAGS = -Wall -Werror -Wextra -std=c89
EXE = master.out

#
# Project files
#
SRCS = master.c nave.c porto.c meteo.c
OBJS = $(SRCS:.c=.out)

#
# Debug build settings
#
DBGDIR = debug
DBGEXE = $(DBGDIR)/$(EXE)
DBGOBJS = $(addprefix $(DBGDIR)/, $(OBJS))
DBGCFLAGS = -g -O0 -DDEBUG -D DEBUG

#
# Release build settings
#
RELDIR = release
RELEXE = $(RELDIR)/$(EXE)
RELOBJS = $(addprefix $(RELDIR)/, $(OBJS))
RELCFLAGS = -O2 -DNDEBUG -D RELEASE

.PHONY: relrun dbgrun default clean debug prep release remake

# Default build
default: clean prep release

relrun:
	./$(RELEXE)

dbgrun:
	./$(DBGEXE)

#
# Debug rules
#
debug: clean prep $(DBGEXE)

$(DBGEXE):
	$(CC) $(CFLAGS) $(DBGCFLAGS) -o $(DBGDIR)/master.out master.c
	$(CC) $(CFLAGS) $(DBGCFLAGS) -o $(DBGDIR)/nave.out nave.c
	$(CC) $(CFLAGS) $(DBGCFLAGS) -o $(DBGDIR)/porto.out porto.c
	$(CC) $(CFLAGS) $(DBGCFLAGS) -o $(DBGDIR)/meteo.out meteo.c
	./$(DBGEXE)

#
# Release rules
#
release: clean prep $(RELEXE)

$(RELEXE):
	$(CC) $(CFLAGS) $(RELCFLAGS) -o $(RELDIR)/master.out master.c
	$(CC) $(CFLAGS) $(RELCFLAGS) -o $(RELDIR)/nave.out nave.c
	$(CC) $(CFLAGS) $(RELCFLAGS) -o $(RELDIR)/porto.out porto.c
	$(CC) $(CFLAGS) $(RELCFLAGS) -o $(RELDIR)/meteo.out meteo.c
	./$(RELEXE)

#
# Other rules
#
prep:
	@mkdir -p $(DBGDIR) $(RELDIR)

remake: clean default

clean:
	rm -f -r $(RELDIR) $(DBGDIR)