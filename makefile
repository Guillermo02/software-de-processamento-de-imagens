# Nome do executável
TARGET = executavel

# Arquivos fonte
SRCS = main.c

# Caminhos das bibliotecas SDL3
SDL3_DIR = libs/SDL3

INCLUDE = -I$(SDL3_DIR)/include
LIBDIR = -L$(SDL3_DIR)/lib

# Flags
CFLAGS = -Wall -Wextra $(INCLUDE)
LDFLAGS = $(LIBDIR) -lSDL3 -lSDL3_image -lSDL3_ttf

# Regra default
all: $(TARGET)

# Como compilar o executável
$(TARGET): $(SRCS)
	$(CC) $(SRCS) -o $(TARGET) $(CFLAGS) $(LDFLAGS)

# Limpar arquivos objeto ou executável
clean:
	rm -f $(TARGET) *.o
