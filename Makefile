CC = gcc
CFLAGS = -Wall -g -Iinclude

# Percorsi
OBJDIR = obj
BINDIR = bin

# Sorgenti e oggetti (scritti a mano)
SRCS = main.c src/ipc_barrier.c 
OBJS = $(OBJDIR)/main.o $(OBJDIR)/ipc_barrier.o $(OBJDIR)/utils.o 

# Nome eseguibile
TARGET = $(BINDIR)/leader_election

# Compilazione di default
all: $(TARGET)

# Link finale
$(TARGET): $(OBJS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $^

# Regole per compilare i .c in obj/
$(OBJDIR)/main.o: main.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/ipc_barrier.o: src/ipc_barrier.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/utils.o: src/utils.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@


# Creazione cartelle se servono
$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

# Pulizia
clean:
	rm -rf $(OBJDIR) $(BINDIR)

exec:
	$(TARGET)

.PHONY: all clean exec
