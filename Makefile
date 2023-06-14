# Makefile
CC = cc
TARGET = ppos

# flags
CPPFLAGS = -Wall --std=c99

# diretorios fonte
VPATH = lib:tests

objs = ppos_core.o queue.o $(TEST).o

.PHONY: all clean purge

all: $(TARGET)
debug: CPPFLAGS += -g -DDEBUG 
debug: $(TARGET)

# ligacao
$(TARGET): $(objs)
	$(CC) $(objs) $(CPPFLAGS) -o $(TARGET)

# compilacao
ppos.o: ppos.h ppos_data.h
queue.o: queue.h
$(TEST).o: ppos.h 

# limpeza
clean:
	-rm -f $(objs) *~ *.o
purge: clean
	-rm -f $(TARGET)
