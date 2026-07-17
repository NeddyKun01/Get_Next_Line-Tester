#include "gnl_tester.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>

static fs::path executable_dir(const char *argv0)
{
	fs::path path(argv0);

	if (path.has_parent_path())
		return (fs::weakly_canonical(path.parent_path()));
	return (fs::current_path());
}

static std::string quote(const std::string &value)
{
	std::string out = "'";

	for (char c : value)
	{
		if (c == '\'')
			out += "'\\''";
		else
			out += c;
	}
	out += "'";
	return (out);
}

static int run_command(const std::string &cmd, const fs::path &log)
{
	std::string full = cmd + " > " + quote(log.string()) + " 2>&1";
	int status = std::system(full.c_str());

	if (status == -1)
		return (127);
	if (WIFEXITED(status))
		return (WEXITSTATUS(status));
	return (128);
}

static std::vector<int> parse_buffers(const std::string &text)
{
	std::vector<int> values;
	std::stringstream ss(text);
	std::string item;

	while (std::getline(ss, item, ','))
	{
		int value = std::atoi(item.c_str());
		if (value > 0)
			values.push_back(value);
	}
	std::sort(values.begin(), values.end());
	values.erase(std::unique(values.begin(), values.end()), values.end());
	return (values);
}

static void print_help(void)
{
	std::cout
		<< "Get Next Line Tester\n\n"
		<< "Usage: ./gnl_tester --root /path/to/Get_Next_Line [options]\n\n"
		<< "Options:\n"
		<< "  --root PATH       target project root (default: ..)\n"
		<< "  --bonus          test get_next_line_bonus files\n"
		<< "  --buffer LIST    comma-separated BUFFER_SIZE values\n"
		<< "  --quick          use BUFFER_SIZE=1,42\n"
		<< "  --strict         use a wider BUFFER_SIZE matrix\n"
		<< "  --leaks          run each suite with valgrind when available\n"
		<< "  --no-color       disable colors\n"
		<< "  --help           show this help\n";
}

static Config parse_args(int argc, char **argv)
{
	Config cfg;

	for (int i = 1; i < argc; ++i)
	{
		std::string arg(argv[i]);
		if (arg == "--root" && i + 1 < argc)
			cfg.root = argv[++i];
		else if (arg.rfind("--root=", 0) == 0)
			cfg.root = arg.substr(7);
		else if (arg == "--bonus")
			cfg.bonus = true;
		else if (arg == "--leaks")
			cfg.leaks = true;
		else if (arg == "--no-color")
			cfg.color = false;
		else if (arg == "--quick")
			cfg.buffers = {1, 42};
		else if (arg == "--strict")
			cfg.buffers = {1, 2, 3, 4, 5, 7, 8, 16, 32, 42, 64, 128, 1024};
		else if (arg == "--buffer" && i + 1 < argc)
			cfg.buffers = parse_buffers(argv[++i]);
		else if (arg.rfind("--buffer=", 0) == 0)
			cfg.buffers = parse_buffers(arg.substr(9));
		else if (arg == "--help" || arg == "-h")
			cfg.help = true;
	}
	if (cfg.buffers.empty())
		cfg.buffers = {42};
	return (cfg);
}

static std::string read_file(const fs::path &path)
{
	std::ifstream in(path);
	std::ostringstream ss;

	ss << in.rdbuf();
	return (ss.str());
}

static bool validate_root(const Config &cfg)
{
	std::vector<fs::path> required;

	if (cfg.bonus)
		required = {"get_next_line_bonus.c", "get_next_line_utils_bonus.c",
			"get_next_line_bonus.h"};
	else
		required = {"get_next_line.c", "get_next_line_utils.c",
			"get_next_line.h"};
	for (const fs::path &name : required)
	{
		if (!fs::exists(cfg.root / name))
		{
			std::cerr << "Missing " << (cfg.root / name) << "\n";
			return (false);
		}
	}
	return (true);
}

static fs::path harness_path(const fs::path &tester_dir, bool bonus)
{
	if (bonus)
		return (tester_dir / "tester" / "tests" / "bonus_harness.c");
	return (tester_dir / "tester" / "tests" / "mandatory_harness.c");
}

static fs::path harness_utils_path(const fs::path &tester_dir)
{
	return (tester_dir / "tester" / "tests" / "harness_utils.c");
}

static std::string compile_command(const Config &cfg, const fs::path &harness,
	const fs::path &utils, const fs::path &exe, int buffer)
{
	std::ostringstream cmd;
	fs::path tests_dir = harness.parent_path();

	cmd << "cc -Wall -Wextra -Werror -D BUFFER_SIZE=" << buffer
		<< " -I " << quote(cfg.root.string())
		<< " -I " << quote(tests_dir.string()) << " "
		<< quote(harness.string()) << " "
		<< quote(utils.string()) << " ";
	if (cfg.bonus)
		cmd << quote((cfg.root / "get_next_line_bonus.c").string()) << " "
			<< quote((cfg.root / "get_next_line_utils_bonus.c").string()) << " ";
	else
		cmd << quote((cfg.root / "get_next_line.c").string()) << " "
			<< quote((cfg.root / "get_next_line_utils.c").string()) << " ";
	cmd << " -o " << quote(exe.string());
	return (cmd.str());
}

static bool command_exists(const std::string &name)
{
	std::string cmd = "command -v " + quote(name) + " >/dev/null 2>&1";
	return (std::system(cmd.c_str()) == 0);
}

static RunResult run_one(const Config &cfg, const fs::path &build,
	const fs::path &harness, const fs::path &utils, int buffer)
{
	RunResult res;
	fs::path run_dir = build / ("buffer_" + std::to_string(buffer)
		+ (cfg.bonus ? "_bonus" : ""));
	fs::path exe = run_dir / "gnl_case";
	fs::path compile_log = run_dir / "compile.log";
	fs::path test_log = run_dir / "test.log";
	fs::path leak_log = run_dir / "leaks.log";
	int code;

	res.buffer = buffer;
	fs::create_directories(run_dir);
	code = run_command(compile_command(cfg, harness, utils, exe, buffer), compile_log);
	res.compile_output = read_file(compile_log);
	res.compile_ok = (code == 0);
	if (!res.compile_ok)
		return (res);
	code = run_command(quote(exe.string()) + " " + quote(run_dir.string()), test_log);
	res.test_output = read_file(test_log);
	res.tests_ok = (code == 0);
	if (cfg.leaks && command_exists("valgrind"))
	{
		std::string cmd = "valgrind --leak-check=full --errors-for-leak-kinds=all "
			"--error-exitcode=42 " + quote(exe.string()) + " "
			+ quote(run_dir.string());
		code = run_command(cmd, leak_log);
		res.leak_output = read_file(leak_log);
		res.leaks_ok = (code == 0);
	}
	return (res);
}

static std::string paint(const Config &cfg, const char *code)
{
	if (!cfg.color)
		return ("");
	return (code);
}

static void print_result(const Config &cfg, const RunResult &res)
{
	bool ok = res.compile_ok && res.tests_ok && res.leaks_ok;

	std::cout << (ok ? paint(cfg, "\033[32m") : paint(cfg, "\033[31m"))
		<< (ok ? "OK " : "NOK") << paint(cfg, "\033[0m")
		<< " BUFFER_SIZE=" << res.buffer;
	if (cfg.bonus)
		std::cout << " bonus";
	std::cout << "\n";
	if (!res.compile_ok)
		std::cout << res.compile_output;
	else if (!res.tests_ok)
		std::cout << res.test_output;
	else if (!res.leaks_ok)
		std::cout << res.leak_output;
}

int main(int argc, char **argv)
{
	Config cfg = parse_args(argc, argv);
	fs::path tester_dir = executable_dir(argv[0]);
	fs::path build = tester_dir / "tester" / "build" / "runs";
	fs::path harness;
	fs::path harness_utils;
	int passed = 0;

	if (cfg.help)
	{
		print_help();
		return (0);
	}
	cfg.root = fs::weakly_canonical(cfg.root);
	if (!validate_root(cfg))
		return (2);
	harness = harness_path(tester_dir, cfg.bonus);
	harness_utils = harness_utils_path(tester_dir);
	if (!fs::exists(harness))
	{
		std::cerr << "Missing " << harness << "\n";
		return (2);
	}
	if (!fs::exists(harness_utils))
	{
		std::cerr << "Missing " << harness_utils << "\n";
		return (2);
	}
	fs::create_directories(build);
	std::cout << "Get Next Line Tester\n";
	std::cout << "target: " << cfg.root << "\n";
	std::cout << "mode:   " << (cfg.bonus ? "bonus" : "mandatory") << "\n\n";
	for (int buffer : cfg.buffers)
	{
		RunResult res = run_one(cfg, build, harness, harness_utils, buffer);
		if (res.compile_ok && res.tests_ok && res.leaks_ok)
			passed++;
		print_result(cfg, res);
	}
	std::cout << "\nSummary: " << passed << "/" << cfg.buffers.size()
		<< " buffer suites passed\n";
	if (cfg.leaks && !command_exists("valgrind"))
		std::cout << "Note: valgrind not found, leak checks were skipped.\n";
	return (passed == static_cast<int>(cfg.buffers.size()) ? 0 : 1);
}
