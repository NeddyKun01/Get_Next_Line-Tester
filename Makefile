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

doctor: build
	@./$(NAME) --root "$(ROOT_DIR)" --doctor

presets: build
	@./$(NAME) --presets

review: build
	@./$(NAME) --root "$(ROOT_DIR)" --preset review $(ARGS)

report-json: build
	@./$(NAME) --root "$(ROOT_DIR)" --preset ci --output gnl-test-report.json $(ARGS)

report-web: build
	@./$(NAME) --root "$(ROOT_DIR)" --preset web --output gnl-test-report.html $(ARGS)

self-test: build
	@mkdir -p $(BUILD_DIR)/self-test
	@./$(NAME) --version
	@./$(NAME) --presets
	@./$(NAME) --list
	@./$(NAME) --cases
	@./$(NAME) --coverage
	@./$(NAME) --root tester/fixtures/good --doctor
	@./$(NAME) --root tester/fixtures/good --doctor --json --output $(BUILD_DIR)/self-test/doctor.json
	@./$(NAME) --root tester/fixtures/good --doctor --web --output $(BUILD_DIR)/self-test/doctor.html
	@./$(NAME) --root tester/fixtures/good --diagnose
	@if ./$(NAME) --root tester/fixtures/good --diagnose --norm > $(BUILD_DIR)/self-test/norm.log 2>&1; then \
		echo "norm diagnostics passed"; \
	else \
		echo "norm diagnostics produced findings as expected"; \
	fi
	@grep -q 'Norminette' $(BUILD_DIR)/self-test/norm.log
	@./$(NAME) --root tester/fixtures/good --health
	@./$(NAME) --explain stdin
	@./$(NAME) --export-fixture double-buffer --buffer 42 --output $(BUILD_DIR)/self-test/exported-fixtures
	@if ./$(NAME) --only not-a-case > $(BUILD_DIR)/self-test/invalid-case.log 2>&1; then \
		echo "invalid case unexpectedly passed"; exit 1; \
	else \
		echo "invalid case failed as expected"; \
	fi
	@if ./$(NAME) --root tester/fixtures/broken --doctor > $(BUILD_DIR)/self-test/doctor-broken.log 2>&1; then \
		echo "broken header unexpectedly passed doctor"; exit 1; \
	else \
		echo "broken header failed doctor as expected"; \
	fi
	@grep -q 'missing prototype' $(BUILD_DIR)/self-test/doctor-broken.log
	@./$(NAME) --root tester/fixtures/good --preset review --no-color
	@./$(NAME) --root tester/fixtures/good --review --compact
	@./$(NAME) --root tester/fixtures/good --bonus --strict --summary-only --no-color
	@./$(NAME) --root tester/fixtures/good --bonus --only bonus-wide-fds --fd-limit 12 --quick --summary-only --no-color
	@./$(NAME) --root tester/fixtures/good --only stdin --quick --summary-only --no-color
	@./$(NAME) --root tester/fixtures/good --only double-buffer --preset pedantic --summary-only --no-color
	@./$(NAME) --root tester/fixtures/good --only malloc-fail --quick --summary-only --no-color
	@./$(NAME) --root tester/fixtures/good --skip stdin --quick --summary-only --no-color
	@./$(NAME) --root tester/fixtures/good --quick --profile --summary-only --no-color
	@./$(NAME) --root tester/fixtures/good --preset ci --output $(BUILD_DIR)/self-test/report.json
	@./$(NAME) --root tester/fixtures/good --preset web --output $(BUILD_DIR)/self-test/report.html
	@./$(NAME) --root tester/fixtures/good --compare tester/fixtures/broken --quick --no-color
	@if ./$(NAME) --root tester/fixtures/broken --quick --web --output $(BUILD_DIR)/self-test/broken.html; then \
		echo "broken fixture unexpectedly passed Web report"; exit 1; \
	else \
		echo "broken fixture failed Web report as expected"; \
	fi
	@if ./$(NAME) --root tester/fixtures/broken --quick --summary-only --no-color > $(BUILD_DIR)/self-test/broken.log 2>&1; then \
		echo "broken fixture unexpectedly passed"; exit 1; \
	else \
		echo "broken fixture failed as expected"; \
	fi
	@if ./$(NAME) --root tester/fixtures/broken --quick --json --output $(BUILD_DIR)/self-test/broken.json; then \
		echo "broken fixture unexpectedly passed JSON report"; exit 1; \
	else \
		echo "broken fixture failed JSON report as expected"; \
	fi
	@if ./$(NAME) --root tester/fixtures/broken --rerun-failed $(BUILD_DIR)/self-test/broken.json --summary-only --no-color; then \
		echo "rerun-failed unexpectedly passed"; exit 1; \
	else \
		echo "rerun-failed failed as expected"; \
	fi
	@grep -q '"verdict":"PASS"' $(BUILD_DIR)/self-test/report.json
	@grep -q '"doctor":true' $(BUILD_DIR)/self-test/doctor.json
	@grep -q '"case_filter"' $(BUILD_DIR)/self-test/report.json
	@grep -q '<!doctype html>' $(BUILD_DIR)/self-test/report.html
	@grep -q 'Doctor Report' $(BUILD_DIR)/self-test/doctor.html
	@grep -q 'Diagnostics' $(BUILD_DIR)/self-test/report.html
	@grep -q 'Likely fix' $(BUILD_DIR)/self-test/broken.html
	@test -f $(BUILD_DIR)/self-test/exported-fixtures/buffer_double.txt

build: $(NAME)

$(BUILD_DIR):
	@mkdir -p $@

$(BUILD_DIR)/src_%.o: $(SRC_DIR)/%.cpp $(HEADERS) | $(BUILD_DIR)
	@$(CXX) $(CXXFLAGS) -I$(INC_DIR) -c $< -o $@

$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	@rm -rf $(BUILD_DIR) $(NAME) gnl-test-report.txt gnl-test-report.json gnl-test-report.html

clean-runs:
	@rm -rf $(BUILD_DIR)/run_* $(BUILD_DIR)/review_* $(BUILD_DIR)/compare_*

fclean: clean

re: fclean build

.PHONY: all menu doctor presets review report-json report-web self-test build clean clean-runs fclean re
