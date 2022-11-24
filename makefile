# basic rules
CC 		= gcc
LDFLAGS	= -lm
COMMON 	= common_ipcs.c utils.c
TARGET 	= master.out nave.out porto.out meteo.out
SOURCE 	= master.c nave.c porto.c meteo.c

# directories
DIRSRC 	= source
DIRHDR	= headers
DIRDBG	= debug
DIRRLS	= release

# compiling flags
CFLAGS	= -std=c89 -pedantic
DCFLAGS	= $(CFLAGS) -g -O0 -DDEBUG
RCFLAGS = $(CFLAGS) -O2 -Wall -Wextra -DNDEBUG

# $< is the first prerequisite
# $@ is the target
# $^ is all the prerequisites

.PHONY: all drun run debug release clean

# build all the prerequisites for compile the project
all: clean prep debug release

debug: $(addprefix $(DIRDBG)/, $(TARGET))

$(DIRDBG)/%.out: $(DIRSRC)/%.c
	$(CC) $(DCFLAGS)	$(addprefix $(DIRSRC)/, $(COMMON))	$<	-o	$@	$(LDFLAGS)

release: $(addprefix $(DIRRLS)/, $(TARGET))

$(DIRRLS)/%.out: $(DIRSRC)/%.c
	$(CC) $(RCFLAGS)	$(addprefix $(DIRSRC)/, $(COMMON)) $<	-o	$@	$(LDFLAGS)

drun: $(addprefix $(DIRDBG)/, $(TARGET))
	./$(DIRDBG)/$(TARGET)

# Use make run to run the executable manually. CLion, when running all automatically runs the executable at the end.
run: $(addprefix $(DIRRLS)/, $(TARGET))
	./$(DIRRLS)/$(TARGET)

prep:
	@mkdir -p $(DIRDBG) $(DIRRLS)

# remove all files and directories in the debug and release folders
clean:
	rm -rf $(DIRDBG)/* $(DIRRLS)/*