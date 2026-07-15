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
	bool bonus = false;
	bool leaks = false;
	bool color = true;
	bool help = false;
};

struct RunResult
{
	int buffer = 0;
	bool compile_ok = false;
	bool tests_ok = false;
	bool leaks_ok = true;
	std::string compile_output;
	std::string test_output;
	std::string leak_output;
};

#endif
