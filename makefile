# basic rules
CC 		= gcc
LDFLAGS	= -lm
COMMON 	= common_ipcs.c utils.c
TARGET 	= master
SOURCE 	= master.c nave.c porto.c meteo.c

# directories
DIRSRC 	= source
DIRHDR	= headers
DIRDBG	= debug
DIRRLS	= release

# compiling flags
CFLAGS	= -std=c89
DCFLAGS	= $(CFLAGS) -g -O0 -pedantic -DDEBUG
RCFLAGS = $(CFLAGS) -O2 -Wall -Wextra -DNDEBUG

# $< is the first prerequisite
# $@ is the target
# $^ is all the prerequisites

.PHONY: all prep rls dbg drun run clean

# build all the prerequisites for compiling the project
all: clean prep rls

prep:
	@mkdir -p $(DIRDBG) $(DIRRLS)

rls: $(addprefix $(DIRSRC)/, $(SOURCE))
	$(CC) $(RCFLAGS) $(addprefix $(DIRSRC)/, $(COMMON)) $(DIRSRC)/nave.c -o $(DIRRLS)/nave.o $(LDFLAGS)
	$(CC) $(RCFLAGS) $(addprefix $(DIRSRC)/, $(COMMON)) $(DIRSRC)/porto.c -o $(DIRRLS)/porto.o $(LDFLAGS)
	$(CC) $(RCFLAGS) $(addprefix $(DIRSRC)/, $(COMMON)) $(DIRSRC)/meteo.c -o $(DIRRLS)/meteo.o $(LDFLAGS)
	$(CC) $(RCFLAGS) $(addprefix $(DIRSRC)/, $(COMMON)) $(DIRSRC)/master.c -o $(DIRRLS)/$(TARGET) $(LDFLAGS)


dbg: $(addprefix $(DIRSRC)/, $(SOURCE))
	$(CC) $(DCFLAGS) $(addprefix $(DIRSRC)/, $(COMMON)) $(DIRSRC)/nave.c -o $(DIRDBG)/nave.o $(LDFLAGS)
	$(CC) $(DCFLAGS) $(addprefix $(DIRSRC)/, $(COMMON)) $(DIRSRC)/porto.c -o $(DIRDBG)/porto.o $(LDFLAGS)
	$(CC) $(DCFLAGS) $(addprefix $(DIRSRC)/, $(COMMON)) $(DIRSRC)/meteo.c -o $(DIRDBG)/meteo.o $(LDFLAGS)
	$(CC) $(DCFLAGS) $(addprefix $(DIRSRC)/, $(COMMON)) $(DIRSRC)/master.c -o $(DIRDBG)/$(TARGET) $(LDFLAGS)

drun: $(DIRDBG)/$(TARGET)
	./$(DIRDBG)/$(TARGET)

# Use make run to run the executable manually. CLion, when running all automatically runs the executable at the end.
run: $(DIRRLS)/$(TARGET)
	./$(DIRRLS)/$(TARGET)

clean:
	# remove all files and directories in the debug and release folders
	rm -rf $(DIRDBG)/*
	rm -rf $(DIRRLS)/*
