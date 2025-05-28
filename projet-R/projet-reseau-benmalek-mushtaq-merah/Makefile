CC = gcc
CFLAGS = -Wall -Wextra -g -I./src/headers -I./src/headers/structs

SRCDIR = src
LAUNCHDIR = $(SRCDIR)/launch
SALEDIR = $(SRCDIR)/sale
MESSDIR = $(SRCDIR)/messages
BUILDDIR = build
CONSENSUSDIR = $(SRCDIR)/consensus

SOURCES = \
	$(LAUNCHDIR)/network.c \
	$(LAUNCHDIR)/peers.c \
	$(LAUNCHDIR)/join.c \
	$(SALEDIR)/sale_process.c \
	$(SALEDIR)/sales.c \
	$(MESSDIR)/struct_mess.c \
	$(MESSDIR)/messages.c \
	$(SRCDIR)/udp_message_recv.c \
	$(CONSENSUSDIR)/consensus.c \
	$(SRCDIR)/main.c \
	$(SRCDIR)/utils.c

OBJECTS = $(patsubst %.c,$(BUILDDIR)/%.o,$(notdir $(SOURCES)))

TARGET = auctionP2P

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(addprefix $(BUILDDIR)/,$(notdir $(OBJECTS)))


$(BUILDDIR)/%.o: $(LAUNCHDIR)/%.c
	mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(MESSDIR)/%.c
	mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(SALEDIR)/%.c
	mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(CONSENSUSDIR)/%.c
	mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -rf $(BUILDDIR) $(TARGET)

# Rebuild from scratch
rebuild: clean all