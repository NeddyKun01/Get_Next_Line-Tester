#ifndef GNL_TESTER_HPP
# define GNL_TESTER_HPP

# include <filesystem>
# include <string>
# include <vector>

namespace fs = std::filesystem;

struct Config
{
	fs::path root = "..";
	fs::path output;
	fs::path compare_root;
	fs::path rerun_failed;
	std::string invalid_preset;
	std::string command;
	std::string only_case;
	std::string skip_case;
	std::string explain_case;
	std::string export_fixture;
	std::string invalid_case;
	std::vector<int> buffers = {1, 2, 3, 5, 8, 16, 32, 42, 128, 1024};
	int timeout_ms = 3000;
	int fd_limit = 32;
	bool bonus = false;
	bool review = false;
	bool stress = false;
	bool leaks = false;
	bool norm = false;
	bool profile = false;
	bool compact = false;
	bool malloc_fail = false;
	bool summary_only = false;
	bool fail_fast = false;
	bool json = false;
	bool web = false;
	bool doctor = false;
	bool diagnose = false;
	bool health = false;
	bool presets = false;
	bool version = false;
	bool list = false;
	bool cases = false;
	bool coverage = false;
	bool coverage_md = false;
	bool keep_build = false;
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
	long long duration_ms = 0;
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
	long long duration_ms = 0;
	int passed = 0;
	int total = 0;
	std::vector<std::string> leak_issues;
	std::vector<RunResult> results;
};

#endif
