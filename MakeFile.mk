# Compiler
GPP = g++

# Compilation flags
GPPFlags = -Wall -Wextra -Werror -Wfatal-errors -std=c++23 -O3
Debug = -g

SRC = $(wildcard *.cpp)

HEADER = $(wildcard *.h)

OBJ = $(SRC:.cpp=.o)

# Target Name
TARGET = main

# compile
$(OBJ): $(SRC) $(OBJ)
	$(GPP) $(GPPFlags) $(SRC) -o $(TARGET)

# clean
clean:
	rm -f $(TARGET) $(OBJ)
	@echo "Removed all object files."

all: clean
	$(GPP) $(GPPFlags) main.cpp -o $(TARGET)
	$(GPP) $(GPPFlags) testing.cpp -o testing