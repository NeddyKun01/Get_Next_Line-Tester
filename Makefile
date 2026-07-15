CXX=c++
CXXFLAGS=-std=c++17 -Wall -Wextra -Werror

.DEFAULT_GOAL := all
MAKEFLAGS += --no-print-directory

ROOT_DIR=..
ARGS=

NAME=gnl_tester
SRC_DIR=tester/src
INC_DIR=tester/include
BUILD_DIR=tester/build

SRCS=$(wildcard $(SRC_DIR)/*.cpp)
OBJS=$(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/src_%.o)
HEADERS=$(INC_DIR)/gnl_tester.hpp

all: build
	@./$(NAME) --root "$(ROOT_DIR)" $(ARGS)

menu: all

build: $(NAME)

$(BUILD_DIR):
	@mkdir -p $@

$(BUILD_DIR)/src_%.o: $(SRC_DIR)/%.cpp $(HEADERS) | $(BUILD_DIR)
	@$(CXX) $(CXXFLAGS) -I$(INC_DIR) -c $< -o $@

$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	@rm -rf $(BUILD_DIR) $(NAME) gnl-test-report.txt

fclean: clean

re: fclean build

.PHONY: all menu build clean fclean re
