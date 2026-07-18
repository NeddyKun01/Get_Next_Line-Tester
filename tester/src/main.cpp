#include "gnl_tester.hpp"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

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

static int run_command_timeout(const std::vector<std::string> &args,
	const fs::path &log, int timeout_ms, bool &timed_out)
{
	std::vector<char *> argv;
	auto start = std::chrono::steady_clock::now();
	pid_t pid;
	int fd;
	int status;

	timed_out = false;
	fd = open(log.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
		return (127);
	pid = fork();
	if (pid < 0)
	{
		close(fd);
		return (127);
	}
	if (pid == 0)
	{
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		close(fd);
		for (const std::string &arg : args)
			argv.push_back(const_cast<char *>(arg.c_str()));
		argv.push_back(NULL);
		execvp(argv[0], argv.data());
		_exit(127);
	}
	close(fd);
	while (true)
	{
		pid_t done = waitpid(pid, &status, WNOHANG);
		if (done == pid)
		{
			if (WIFEXITED(status))
				return (WEXITSTATUS(status));
			if (WIFSIGNALED(status))
				return (128 + WTERMSIG(status));
			return (128);
		}
		if (done < 0)
			return (127);
		if (timeout_ms > 0)
		{
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - start).count();
			if (elapsed >= timeout_ms)
			{
				timed_out = true;
				kill(pid, SIGKILL);
				waitpid(pid, &status, 0);
				return (124);
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
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
		<< "  --review         run mandatory strict and bonus strict when available\n"
		<< "  --stress         enable large-line stress fixtures\n"
		<< "  --buffer LIST    comma-separated BUFFER_SIZE values\n"
		<< "  --quick          use BUFFER_SIZE=1,42\n"
		<< "  --strict         use a wider BUFFER_SIZE matrix\n"
		<< "  --leaks          run each suite with valgrind when available\n"
		<< "  --summary-only   print only compact suite summaries\n"
		<< "  --fail-fast      stop after the first failing buffer suite\n"
		<< "  --timeout MS     kill a test run after this many ms (default: 3000)\n"
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
		else if (arg == "--review")
			cfg.review = true;
		else if (arg == "--stress")
			cfg.stress = true;
		else if (arg == "--leaks")
			cfg.leaks = true;
		else if (arg == "--summary-only")
			cfg.summary_only = true;
		else if (arg == "--fail-fast")
			cfg.fail_fast = true;
		else if (arg == "--no-color")
			cfg.color = false;
		else if (arg == "--quick")
			cfg.buffers = {1, 42};
		else if (arg == "--strict")
			cfg.buffers = {1, 2, 3, 4, 5, 7, 8, 16, 32, 42, 64, 128, 1024};
		else if (arg == "--timeout" && i + 1 < argc)
			cfg.timeout_ms = std::max(0, std::atoi(argv[++i]));
		else if (arg.rfind("--timeout=", 0) == 0)
			cfg.timeout_ms = std::max(0, std::atoi(arg.substr(10).c_str()));
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

static bool has_bonus_files(const fs::path &root)
{
	return (fs::exists(root / "get_next_line_bonus.c")
		&& fs::exists(root / "get_next_line_utils_bonus.c")
		&& fs::exists(root / "get_next_line_bonus.h"));
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
	if (cfg.stress)
		cmd << " -D GNL_STRESS=1 ";
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

static void add_unique(std::vector<std::string> &values, const std::string &value)
{
	if (std::find(values.begin(), values.end(), value) == values.end())
		values.push_back(value);
}

static bool has_nonzero_valgrind_count(const std::string &log,
	const std::string &label)
{
	size_t	pos;
	size_t	end;
	std::string	line;

	pos = log.find(label);
	if (pos == std::string::npos)
		return (false);
	end = log.find('\n', pos);
	line = log.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
	return (line.find(": 0 bytes") == std::string::npos
		&& line.find(": 0 allocs") == std::string::npos);
}

static std::vector<std::string> classify_valgrind(const std::string &log)
{
	std::vector<std::string> issues;

	if (log.find("Invalid read") != std::string::npos)
		add_unique(issues, "invalid read");
	if (log.find("Invalid write") != std::string::npos)
		add_unique(issues, "invalid write");
	if (log.find("Invalid free") != std::string::npos
		|| log.find("Mismatched free") != std::string::npos)
		add_unique(issues, "invalid free");
	if (log.find("Use of uninitialised value") != std::string::npos
		|| log.find("Conditional jump or move depends on uninitialised")
			!= std::string::npos)
		add_unique(issues, "uninitialised value");
	if (has_nonzero_valgrind_count(log, "definitely lost:"))
		add_unique(issues, "definitely lost");
	if (has_nonzero_valgrind_count(log, "indirectly lost:"))
		add_unique(issues, "indirectly lost");
	if (has_nonzero_valgrind_count(log, "possibly lost:"))
		add_unique(issues, "possibly lost");
	if (has_nonzero_valgrind_count(log, "still reachable:"))
		add_unique(issues, "still reachable");
	return (issues);
}

static std::string join_issues(const std::vector<std::string> &issues)
{
	std::ostringstream out;

	for (size_t i = 0; i < issues.size(); ++i)
	{
		if (i > 0)
			out << ", ";
		out << issues[i];
	}
	return (out.str());
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
	code = run_command_timeout({exe.string(), run_dir.string()}, test_log,
		cfg.timeout_ms, res.timed_out);
	res.test_output = read_file(test_log);
	res.tests_ok = (code == 0 && !res.timed_out);
	if (cfg.leaks && !command_exists("valgrind"))
		res.leak_skipped = true;
	if (cfg.leaks && !res.leak_skipped)
	{
		code = run_command_timeout({
			"valgrind",
			"--leak-check=full",
			"--errors-for-leak-kinds=all",
			"--error-exitcode=42",
			exe.string(),
			run_dir.string()
		}, leak_log, cfg.timeout_ms, res.timed_out);
		res.leak_output = read_file(leak_log);
		res.leak_issues = classify_valgrind(res.leak_output);
		if (code != 0 && !res.timed_out && res.leak_issues.empty())
			add_unique(res.leak_issues, "valgrind error");
		res.leaks_ok = (code == 0 && !res.timed_out
			&& res.leak_issues.empty());
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
	if (res.timed_out)
		std::cout << " timeout(" << cfg.timeout_ms << "ms)";
	std::cout << "\n";
	if (!res.compile_ok)
		std::cout << res.compile_output;
	else if (res.timed_out)
		std::cout << "Test run exceeded " << cfg.timeout_ms << "ms.\n";
	else if (!res.tests_ok)
		std::cout << res.test_output;
	else if (!res.leaks_ok)
	{
		if (!res.leak_issues.empty())
			std::cout << "Valgrind: NOK " << join_issues(res.leak_issues)
				<< "\n";
		std::cout << res.leak_output;
	}
}

static SuiteSummary run_suite(const Config &cfg, const fs::path &tester_dir,
	const fs::path &build, bool bonus, bool verbose)
{
	Config suite_cfg = cfg;
	fs::path harness;
	fs::path harness_utils;
	SuiteSummary summary;

	suite_cfg.bonus = bonus;
	summary.name = bonus ? "Bonus" : "Mandatory";
	if (!validate_root(suite_cfg))
		return (summary);
	harness = harness_path(tester_dir, bonus);
	harness_utils = harness_utils_path(tester_dir);
	if (!fs::exists(harness))
	{
		std::cerr << "Missing " << harness << "\n";
		return (summary);
	}
	if (!fs::exists(harness_utils))
	{
		std::cerr << "Missing " << harness_utils << "\n";
		return (summary);
	}
	for (int buffer : suite_cfg.buffers)
	{
		RunResult res = run_one(suite_cfg, build, harness, harness_utils, buffer);
		if (res.compile_ok && res.tests_ok && res.leaks_ok)
			summary.passed++;
		summary.total++;
		if (suite_cfg.leaks)
		{
			if (res.leak_skipped)
				summary.leak_skipped = true;
			else if (res.compile_ok && res.tests_ok)
				summary.leak_checked = true;
			for (const std::string &issue : res.leak_issues)
				add_unique(summary.leak_issues, issue);
		}
		if (!suite_cfg.summary_only
			&& (verbose || !(res.compile_ok && res.tests_ok && res.leaks_ok)))
			print_result(suite_cfg, res);
		if (suite_cfg.fail_fast
			&& !(res.compile_ok && res.tests_ok && res.leaks_ok))
			break ;
	}
	summary.success = (summary.total > 0 && summary.passed == summary.total);
	return (summary);
}

static void print_review_line(const Config &cfg, const SuiteSummary &summary)
{
	std::string label = summary.name + ":";
	bool ok = summary.success && !summary.skipped;

	std::cout << label;
	if (label.size() < 11)
		std::cout << std::string(11 - label.size(), ' ');
	if (summary.skipped)
	{
		std::cout << "SKIP\n";
		return ;
	}
	std::cout << (ok ? paint(cfg, "\033[32m") : paint(cfg, "\033[31m"))
		<< (ok ? "OK " : "NOK") << paint(cfg, "\033[0m")
		<< summary.passed << "/" << summary.total << "\n";
}

static void merge_leak_summary(SuiteSummary &target, const SuiteSummary &source)
{
	if (source.leak_checked)
		target.leak_checked = true;
	if (source.leak_skipped)
		target.leak_skipped = true;
	for (const std::string &issue : source.leak_issues)
		add_unique(target.leak_issues, issue);
}

static void print_valgrind_review_line(const Config &cfg,
	const SuiteSummary &summary)
{
	bool ok = summary.leak_checked && summary.leak_issues.empty()
		&& !summary.leak_skipped;

	std::cout << "Valgrind: ";
	if (!cfg.leaks)
	{
		std::cout << "SKIP\n";
		return ;
	}
	if (summary.leak_skipped && !summary.leak_checked)
	{
		std::cout << "SKIP not installed\n";
		return ;
	}
	std::cout << (ok ? paint(cfg, "\033[32m") : paint(cfg, "\033[31m"))
		<< (ok ? "OK" : "NOK") << paint(cfg, "\033[0m");
	if (!summary.leak_issues.empty())
		std::cout << " " << join_issues(summary.leak_issues);
	std::cout << "\n";
}

static int run_review(Config cfg, const fs::path &tester_dir)
{
	fs::path build = tester_dir / "tester" / "build" / "review";
	SuiteSummary mandatory;
	SuiteSummary bonus;
	SuiteSummary leak_summary;
	bool bonus_available;
	bool pass;

	cfg.buffers = {1, 2, 3, 4, 5, 7, 8, 16, 32, 42, 64, 128, 1024};
	bonus_available = has_bonus_files(cfg.root);
	fs::create_directories(build);
	std::cout << "Get Next Line Tester Review\n\n";
	std::cout << "target:  " << cfg.root << "\n";
	std::cout << "timeout: " << cfg.timeout_ms << "ms\n";
	std::cout << "stress:  " << (cfg.stress ? "enabled" : "skipped") << "\n";
	std::cout << "leaks:   " << (cfg.leaks ? "enabled" : "skipped") << "\n\n";
	mandatory = run_suite(cfg, tester_dir, build, false, false);
	if (bonus_available)
		bonus = run_suite(cfg, tester_dir, build, true, false);
	else
	{
		bonus.name = "Bonus";
		bonus.skipped = true;
	}
	std::cout << "\n";
	print_review_line(cfg, mandatory);
	print_review_line(cfg, bonus);
	merge_leak_summary(leak_summary, mandatory);
	if (!bonus.skipped)
		merge_leak_summary(leak_summary, bonus);
	print_valgrind_review_line(cfg, leak_summary);
	pass = mandatory.success && (bonus.skipped || bonus.success);
	std::cout << "Verdict:  "
		<< (pass ? paint(cfg, "\033[32m") : paint(cfg, "\033[31m"))
		<< (pass ? "PASS" : "FAIL") << paint(cfg, "\033[0m") << "\n";
	return (pass ? 0 : 1);
}

int main(int argc, char **argv)
{
	Config cfg = parse_args(argc, argv);
	fs::path tester_dir = executable_dir(argv[0]);
	fs::path build = tester_dir / "tester" / "build" / "runs";
	fs::path harness;
	fs::path harness_utils;
	int passed = 0;
	int total = 0;

	if (cfg.help)
	{
		print_help();
		return (0);
	}
	cfg.root = fs::weakly_canonical(cfg.root);
	if (cfg.review)
		return (run_review(cfg, tester_dir));
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
		total++;
		if (!cfg.summary_only || !(res.compile_ok && res.tests_ok && res.leaks_ok))
			print_result(cfg, res);
		if (cfg.fail_fast && !(res.compile_ok && res.tests_ok && res.leaks_ok))
			break ;
	}
	std::cout << "\nSummary: " << passed << "/" << total
		<< " buffer suites passed\n";
	if (cfg.fail_fast && total < static_cast<int>(cfg.buffers.size()))
		std::cout << "Stopped early after first failing buffer suite.\n";
	if (cfg.leaks && !command_exists("valgrind"))
		std::cout << "Note: valgrind not found, leak checks were skipped.\n";
	return (passed == total && total == static_cast<int>(cfg.buffers.size())
		? 0 : 1);
}
