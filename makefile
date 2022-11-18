#
# Compiler flags
#
CC     = gcc
CFLAGS = -Wall -std=c89
EXE = master.out

#
# Project files
# TODO: compilatore deve prendere gli oggetti corretti
SRCS = master.c nave.c porto.c meteo.c utils/source/utils.c utils/source/common_ipcs.c
OBJS = $(SRCS:.c=.out)

#
# Debug build settings
#
DBGDIR = debug
DBGEXE = $(DBGDIR)/$(EXE)
DBGOBJS = $(addprefix $(DBGDIR)/, $(OBJS))
DBGCFLAGS = -g -O0 -pedantic -DDEBUG

#
# Release build settings
#
RELDIR = release
RELEXE = $(RELDIR)/$(EXE)
RELOBJS = $(addprefix $(RELDIR)/, $(OBJS))
RELCFLAGS = -O2 -Werror -Wextra -DNDEBUG

.PHONY: relrun dbgrun default clean debug prep release remake

# Default build
default: clean prep release

# Use these to build the debug and release versions manually (if you want)
# CLion's start (play) button is configured to run the executable automatically
run:
	./$(RELEXE)

drun:
	./$(DBGEXE)

#
# Debug rules
#
debug: prep $(DBGEXE)

$(DBGEXE):
	$(CC) $(CFLAGS) $(DBGCFLAGS) -o $(DBGDIR)/master.out master.c
	$(CC) $(CFLAGS) $(DBGCFLAGS) -o $(DBGDIR)/nave.out  nave.c
	$(CC) $(CFLAGS) $(DBGCFLAGS) -o $(DBGDIR)/porto.out porto.c
	$(CC) $(CFLAGS) $(DBGCFLAGS) -o $(DBGDIR)/meteo.out meteo.c


#
# Release rules
#
release: prep $(RELEXE)

$(RELEXE):
	$(CC) $(CFLAGS) $(RELCFLAGS) -o $(RELDIR)/master.out master.c
	$(CC) $(CFLAGS) $(RELCFLAGS) -o $(RELDIR)/nave.out nave.c
	$(CC) $(CFLAGS) $(RELCFLAGS) -o $(RELDIR)/porto.out porto.c
	$(CC) $(CFLAGS) $(RELCFLAGS) -o $(RELDIR)/meteo.out meteo.c

#
# Other rules
#
prep:
	@mkdir -p $(DBGDIR) $(RELDIR)

remake: clean default

clean:
	echo "Cleaning..."
	rm -f -r $(RELDIR) $(DBGDIR)