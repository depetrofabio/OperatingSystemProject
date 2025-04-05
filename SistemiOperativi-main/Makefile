CC = gcc
CFLAGS = -Wall -Wextra -lrt -g
SRCDIR = src
OUTDIR = bin

TARGETS = $(OUTDIR)/master $(OUTDIR)/atomo $(OUTDIR)/attivatore $(OUTDIR)/alimentazione

COMMON_SRC = $(SRCDIR)/common.c

.SILENT: clean run

all: $(TARGETS)

$(OUTDIR)/master: $(SRCDIR)/master.c $(COMMON_SRC)
	$(CC) $(CFLAGS) $^ -o $@
	
$(OUTDIR)/atomo: $(SRCDIR)/atomo.c $(COMMON_SRC)
	$(CC) $(CFLAGS) $^ -o $@

$(OUTDIR)/attivatore: $(SRCDIR)/attivatore.c $(COMMON_SRC)
	$(CC) $(CFLAGS) $^ -o $@

$(OUTDIR)/alimentazione: $(SRCDIR)/alimentazione.c $(COMMON_SRC)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	@rm -f $(TARGETS) && clear

run: $(TARGETS)
	@./$(OUTDIR)/master





