#ifndef GNL_TESTER_HPP
# define GNL_TESTER_HPP

# include <filesystem>
# include <string>
# include <vector>

namespace fs = std::filesystem;

struct Config
{
	fs::path root = "..";
	std::vector<int> buffers = {1, 2, 3, 5, 8, 16, 32, 42, 128, 1024};
	int timeout_ms = 3000;
	bool bonus = false;
	bool review = false;
	bool stress = false;
	bool leaks = false;
	bool summary_only = false;
	bool fail_fast = false;
	bool color = true;
	bool help = false;
};

struct RunResult
{
	int buffer = 0;
	bool compile_ok = false;
	bool tests_ok = false;
	bool leaks_ok = true;
	bool leak_skipped = false;
	bool timed_out = false;
	std::string compile_output;
	std::string test_output;
	std::string leak_output;
	std::vector<std::string> leak_issues;
};

struct SuiteSummary
{
	std::string name;
	bool skipped = false;
	bool success = false;
	bool leak_checked = false;
	bool leak_skipped = false;
	int passed = 0;
	int total = 0;
	std::vector<std::string> leak_issues;
};

#endif
