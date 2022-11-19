CC       = gcc
CFLAGS   = -g3 -Wall
LDFLAGS  =
OBJ = common_ipcs.c utils.c
TARGET = master

# $< is the first prerequisite
# $@ is the target
# $^ is all the prerequisites

all: clean nave porto meteo $(TARGET)

nave: source/nave.c
	$(CC) $(CFLAGS) $(addprefix source/, $(OBJ)) $< -o $(addprefix source/, $@.o)

porto: source/porto.c
	$(CC) $(CFLAGS) $(addprefix source/, $(OBJ)) $< -o $(addprefix source/, $@.o)

meteo: source/meteo.c
	$(CC) $(CFLAGS) $(addprefix source/, $(OBJ)) $< -o $(addprefix source/, $@.o)


$(TARGET): source/master.c
	$(CC) $(LDFLAGS) -o $@ $^

# Use make run to run the executable manually. CLion, when running all automatically runs the executable at the end.
run:
	./$(TARGET)
clean:
	rm -f source/*.o $(TARGET) *~