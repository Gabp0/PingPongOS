# Makefile
CC = cc
TARGET = ppos

# flags
CPPFLAGS = -Wall -g --std=c99
LDLIBS = -lrt -lm

# diretorios fonte
VPATH = lib:tests

objs = ppos_core.o ppos_disk.o queue.o disk.o $(TEST).o

.PHONY: all clean purge debug

all: $(TARGET)
debug: CPPFLAGS += -DDEBUG 
debug: $(TARGET)

# ligacao
$(TARGET): $(objs)
	$(CC) -o $(TARGET) $(objs) $(CPPFLAGS) $(LDLIBS) 

# compilacao
ppos.o: ppos.h ppos_data.h
ppos_disk.o: ppos.h ppos_data.h
queue.o: queue.h
disk.o: disk.h
$(TEST).o: ppos.h 

# limpeza
clean:
	-rm -f $(objs) *~ *.o
purge: clean
	-rm -f $(TARGET)
