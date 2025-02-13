# Define the compiler
CC = arm-linux-gnueabihf-gcc

# Define compiler flags
CFLAGS = -Wall -Iinc

# Define the linker flags
LDFLAGS = -pthread

# Define the target executable and its directory
BIN_DIR = bin
TARGET = $(BIN_DIR)/test

# Define directories
SRC_DIR = src
OBJ_DIR = obj

# Define the source files
SRC = $(wildcard $(SRC_DIR)/*.c)

# Define the object files with the obj
OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

# Default rule
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR) $(OBJ_DIR) # Ensure bin and obj directories exist
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJ)

# Rule to compile .c files into .o files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR) # Ensure obj directory exists
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule to remove compiled files
clean:
	rm -f $(OBJ_DIR)/*.o $(TARGET)

# Run rule to execute the server
run: $(TARGET)
	@echo "Running server..."
	@$(TARGET) 