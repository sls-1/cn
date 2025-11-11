CC           := gcc
FORMATTER    := clang-format

# Compiler flags
CFLAGS       := -Wall -Wextra -Werror -Wpedantic -std=c23 -g -fsanitize=address,undefined

# Formatting style
FORMAT_STYLE := -style="{BasedOnStyle: LLVM, IndentWidth: 4}"

# Targets
CLIENT_SRC   := client.c
SERVER_SRC   := server.c
CLIENT_BIN   := client.out
SERVER_BIN   := server.out

# Default target
all: $(CLIENT_BIN) $(SERVER_BIN)

# Build rules
$(CLIENT_BIN): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(SERVER_BIN): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $^

# Format all source files
format:
	@echo "Formatting source files..."
	@$(FORMATTER) $(FORMAT_STYLE) -i $(CLIENT_SRC) $(SERVER_SRC)

# Clean up build artifacts
clean:
	@echo "Cleaning up..."
	@$(RM) $(CLIENT_BIN) $(SERVER_BIN) *.o

# Phony targets
.PHONY: all clean format
