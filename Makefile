CC := g++
SRCDIR := src
OBJDIR := build

SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name '*.$(SRCEXT)')
OBJECTS := $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(SOURCES:.$(SRCEXT)=.o))

CFLAGS := -g -Wall -std=c++11 -O0
INC := -Iinclude -lpthread

MAIN_CLIENT := client.cpp
MAIN_SERVER := server.cpp

OBJ_CLIENT := cliente
OBJ_SERVER := servidor

$(OBJDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

client: $(OBJECTS)
	$(CC) $(MAIN_CLIENT) $(INC) $(CFLAGS) $^ -o $(OBJ_CLIENT)

server: $(OBJECTS)
	$(CC) $(MAIN_SERVER) $(INC) $(CFLAGS) $^ -o $(OBJ_SERVER)

teste: $(OBJECTS)
	$(CC) teste.cpp $(INC) $(CFLAGS) $^ -o teste

all: client server

clean:
	rm $(OBJDIR)/* $(OBJ_CLIENT) $(OBJ_SERVER)