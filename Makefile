CXX=c++
CXXFLAGS=-std=c++17 -Wall -Wextra -Werror

.DEFAULT_GOAL := all
MAKEFLAGS += --no-print-directory

ROOT_DIR=..
ARGS=

NAME=gnl_tester
SRC_DIR=tester/src
BUILD_DIR=tester/build

SRCS=$(SRC_DIR)/main.cpp
OBJS=$(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

all: build
	@./$(NAME) --root "$(ROOT_DIR)" $(ARGS)

menu: all

build: $(NAME)

$(BUILD_DIR):
	@mkdir -p $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	@rm -rf $(BUILD_DIR) $(NAME) gnl-test-report.txt

fclean: clean

re: fclean build

.PHONY: all menu build clean fclean re
