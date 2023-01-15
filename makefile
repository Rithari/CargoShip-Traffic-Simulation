# basic rules
CC 		= gcc
LDFLAGS	= -lm
COMMON 	= common_ipcs.c utils.c linked_list.c
TARGET 	= master.out nave.out porto.out meteo.out
SOURCE 	= master.c nave.c porto.c meteo.c
EXE		= master.out

# directories
DIRSRC 	= source
DIRHDR	= headers
DIRDBG	= debug
DIRRLS	= release

# compiling flags
CFLAGS	= -std=c89 -Wpedantic
DCFLAGS	= $(CFLAGS) -g -O0 -DDEBUG
RCFLAGS = $(CFLAGS) -O2 -Wall -Wextra -DNDEBUG

# compiling args
ARGS 	?= dense, small ships.txt

# $< is the first prerequisite
# $@ is the target
# $^ is all the prerequisites

# build all the prerequisites for compile the project
all: clean prep debug release

debug: $(addprefix $(DIRDBG)/, $(TARGET))

$(DIRDBG)/%.out: $(DIRSRC)/%.c
	$(CC) $(DCFLAGS)	$(addprefix $(DIRSRC)/, $(COMMON))	$<	-o	$@	$(LDFLAGS)

release: $(addprefix $(DIRRLS)/, $(TARGET))

$(DIRRLS)/%.out: $(DIRSRC)/%.c
	$(CC) $(RCFLAGS)	$(addprefix $(DIRSRC)/, $(COMMON)) $<	-o	$@	$(LDFLAGS)

drun: $(addprefix $(DIRDBG)/, $(TARGET))
	./$(DIRDBG)/$(EXE) "$(ARGS)"

# Use make run to run the executable manually. CLion, when running all automatically runs the executable at the end.
run: $(addprefix $(DIRRLS)/, $(TARGET))
	./$(DIRRLS)/$(EXE) "$(ARGS)"

prep:
	@mkdir -p $(DIRDBG) $(DIRRLS)

# remove all files and directories in the debug and release folders
clean:
	rm -rf $(DIRDBG)/* $(DIRRLS)/*
