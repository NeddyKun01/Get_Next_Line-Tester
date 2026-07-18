#include "gnl_tester.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

static const char *g_version = "1.0.0";

struct CaseInfo
{
	const char *name;
	const char *suite;
	const char *description;
};

struct DiagnosticSummary
{
	bool mandatory_ok = false;
	bool bonus_ok = false;
	bool mandatory_header_ok = false;
	bool bonus_header_ok = true;
	bool mandatory_probe_ok = false;
	bool bonus_probe_ok = true;
	bool makefile_ok = false;
	bool makefile_present = false;
	bool norm_checked = false;
	bool norm_ok = true;
	int warnings = 0;
	std::string mandatory_header_detail;
	std::string bonus_header_detail;
	std::string mandatory_probe_detail;
	std::string bonus_probe_detail;
	std::string makefile_detail;
	std::string norm_detail;
};

static const std::vector<CaseInfo> &case_infos(void)
{
	static const std::vector<CaseInfo> cases = {
		{"invalid-fd", "mandatory/bonus", "get_next_line(-1) returns NULL."},
		{"closed-fd", "mandatory", "A closed file descriptor returns NULL."},
		{"empty", "mandatory", "An empty file reaches EOF without a fake line."},
		{"one-line", "mandatory", "Single-line files with and without final newline."},
		{"many", "mandatory", "Several lines are returned in order."},
		{"blank", "mandatory", "Consecutive newline-only lines are preserved."},
		{"newline-only", "mandatory", "A file containing only one newline returns \"\\n\"."},
		{"many-newlines", "mandatory", "A file containing only newlines returns each blank line."},
		{"long", "mandatory", "A long line larger than common buffers is joined correctly."},
		{"buffer-edge", "mandatory", "Lines around BUFFER_SIZE boundaries keep every byte."},
		{"double-buffer", "mandatory", "Lines at BUFFER_SIZE * 2 and * 2 + 1 boundaries."},
		{"read-zero", "mandatory", "Invalid fd handling does not keep stale state after read errors."},
		{"malloc-fail", "mandatory", "Opt-in malloc failure handling returns NULL cleanly."},
		{"pipe", "mandatory", "A pipe-backed file descriptor behaves like a readable fd."},
		{"stdin", "mandatory", "STDIN_FILENO is accepted and read correctly."},
		{"stress", "mandatory", "Optional 10k and 100k line fixtures."},
		{"bonus-basic", "bonus", "Interleaved reads across two file descriptors."},
		{"bonus-many-fds", "bonus", "Round-robin reads across several file descriptors."},
		{"bonus-wide-fds", "bonus", "Round-robin reads across many file descriptors."}
	};

	return (cases);
}

static bool valid_case(const std::string &name)
{
	for (const CaseInfo &info : case_infos())
	{
		if (name == info.name)
			return (true);
	}
	return (false);
}

static fs::path executable_dir(const char *argv0)
{
	fs::path path(argv0);

	if (path.has_parent_path())
		return (fs::weakly_canonical(path.parent_path()));
	return (fs::current_path());
}

static std::string run_id(const std::string &prefix)
{
	std::ostringstream out;
	long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();

	out << prefix << "_" << getpid() << "_" << ms;
	return (out.str());
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

static void apply_preset(Config &cfg, const std::string &name)
{
	if (name == "quick")
	{
		cfg.buffers = {1, 42};
		cfg.timeout_ms = 2000;
		cfg.fail_fast = true;
	}
	else if (name == "normal")
	{
		cfg.buffers = {1, 2, 3, 5, 8, 16, 32, 42, 128, 1024};
		cfg.timeout_ms = 3000;
	}
	else if (name == "strict" || name == "school")
	{
		cfg.buffers = {1, 2, 3, 4, 5, 7, 8, 16, 32, 42, 64, 128, 1024};
		cfg.timeout_ms = 5000;
	}
	else if (name == "pedantic")
	{
		cfg.buffers = {1, 2, 3, 4, 5, 7, 8, 16, 32, 42, 64, 128, 512, 1024, 4096};
		cfg.stress = true;
		cfg.timeout_ms = 15000;
	}
	else if (name == "review")
	{
		cfg.review = true;
		cfg.summary_only = true;
		cfg.timeout_ms = 5000;
	}
	else if (name == "ci")
	{
		cfg.review = true;
		cfg.json = true;
		cfg.summary_only = true;
		cfg.fail_fast = true;
		cfg.color = false;
		cfg.timeout_ms = 5000;
	}
	else if (name == "web" || name == "html")
	{
		cfg.review = true;
		cfg.web = true;
		cfg.summary_only = true;
		cfg.color = false;
	}
	else if (name == "stress")
	{
		cfg.stress = true;
		cfg.buffers = {42, 1024};
		cfg.timeout_ms = 10000;
	}
	else if (name == "leaks")
	{
		cfg.leaks = true;
		cfg.buffers = {1, 42};
		cfg.timeout_ms = 10000;
	}
}

static bool valid_preset(const std::string &name)
{
	return (name == "quick" || name == "normal" || name == "strict"
		|| name == "school" || name == "review" || name == "ci"
		|| name == "web" || name == "html" || name == "stress"
		|| name == "leaks" || name == "pedantic");
}

static std::string trim_copy(const std::string &value)
{
	size_t start = 0;
	size_t end = value.size();

	while (start < end && std::isspace(static_cast<unsigned char>(value[start])))
		start++;
	while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
		end--;
	return (value.substr(start, end - start));
}

static bool json_string_value(const std::string &text, const std::string &key,
	std::string &value)
{
	std::string needle = "\"" + key + "\"";
	size_t pos = text.find(needle);
	size_t colon;
	size_t first;
	size_t second;

	if (pos == std::string::npos)
		return (false);
	colon = text.find(':', pos + needle.size());
	if (colon == std::string::npos)
		return (false);
	first = text.find('"', colon + 1);
	if (first == std::string::npos)
		return (false);
	second = text.find('"', first + 1);
	if (second == std::string::npos)
		return (false);
	value = text.substr(first + 1, second - first - 1);
	return (true);
}

static bool json_int_value(const std::string &text, const std::string &key,
	int &value)
{
	std::string needle = "\"" + key + "\"";
	size_t pos = text.find(needle);
	size_t colon;
	size_t end;
	std::string number;

	if (pos == std::string::npos)
		return (false);
	colon = text.find(':', pos + needle.size());
	if (colon == std::string::npos)
		return (false);
	end = text.find_first_of(",}\n", colon + 1);
	number = trim_copy(text.substr(colon + 1,
		end == std::string::npos ? std::string::npos : end - colon - 1));
	if (number.empty())
		return (false);
	value = std::atoi(number.c_str());
	return (true);
}

static bool json_bool_value(const std::string &text, const std::string &key,
	bool &value)
{
	std::string needle = "\"" + key + "\"";
	size_t pos = text.find(needle);
	size_t colon;
	std::string rest;

	if (pos == std::string::npos)
		return (false);
	colon = text.find(':', pos + needle.size());
	if (colon == std::string::npos)
		return (false);
	rest = trim_copy(text.substr(colon + 1));
	if (rest.rfind("true", 0) == 0)
		value = true;
	else if (rest.rfind("false", 0) == 0)
		value = false;
	else
		return (false);
	return (true);
}

static void load_config_file(Config &cfg)
{
	fs::path path = fs::current_path() / ".gnl-tester.json";
	std::ifstream in(path);
	std::ostringstream ss;
	std::string text;
	std::string value;
	int int_value;
	bool bool_value;

	if (!in)
		return ;
	ss << in.rdbuf();
	text = ss.str();
	if (json_string_value(text, "root", value))
		cfg.root = value;
	if (json_string_value(text, "output", value))
		cfg.output = value;
	if (json_string_value(text, "preset", value))
	{
		if (valid_preset(value))
			apply_preset(cfg, value);
		else
			cfg.invalid_preset = value;
	}
	if (json_string_value(text, "buffers", value))
		cfg.buffers = parse_buffers(value);
	if (json_int_value(text, "timeout_ms", int_value))
		cfg.timeout_ms = std::max(0, int_value);
	if (json_bool_value(text, "bonus", bool_value))
		cfg.bonus = bool_value;
	if (json_bool_value(text, "review", bool_value))
		cfg.review = bool_value;
	if (json_bool_value(text, "stress", bool_value))
		cfg.stress = bool_value;
	if (json_bool_value(text, "leaks", bool_value))
		cfg.leaks = bool_value;
	if (json_bool_value(text, "summary_only", bool_value))
		cfg.summary_only = bool_value;
	if (json_bool_value(text, "fail_fast", bool_value))
		cfg.fail_fast = bool_value;
	if (json_bool_value(text, "keep_build", bool_value))
		cfg.keep_build = bool_value;
	if (json_bool_value(text, "color", bool_value))
		cfg.color = bool_value;
}

static std::string command_line(int argc, char **argv)
{
	std::ostringstream out;

	for (int i = 0; i < argc; ++i)
	{
		if (i > 0)
			out << " ";
		out << quote(argv[i]);
	}
	return (out.str());
}

static void print_presets(void)
{
	std::cout << "Preset   Expands to\n";
	std::cout << "------------------------------------------------------------\n";
	std::cout << std::left << std::setw(9) << "quick"
		<< "--quick --fail-fast --timeout 2000\n";
	std::cout << std::left << std::setw(9) << "normal"
		<< "default buffer matrix\n";
	std::cout << std::left << std::setw(9) << "strict"
		<< "--strict --timeout 5000\n";
	std::cout << std::left << std::setw(9) << "school"
		<< "--strict --timeout 5000\n";
	std::cout << std::left << std::setw(9) << "pedantic"
		<< "--stress --buffer 1,2,3,4,5,7,8,16,32,42,64,128,512,1024,4096 --timeout 15000\n";
	std::cout << std::left << std::setw(9) << "review"
		<< "--review --summary-only --timeout 5000\n";
	std::cout << std::left << std::setw(9) << "ci"
		<< "--review --json --summary-only --fail-fast --timeout 5000\n";
	std::cout << std::left << std::setw(9) << "web"
		<< "--review --web\n";
	std::cout << std::left << std::setw(9) << "html"
		<< "--review --html\n";
	std::cout << std::left << std::setw(9) << "stress"
		<< "--stress --buffer 42,1024 --timeout 10000\n";
	std::cout << std::left << std::setw(9) << "leaks"
		<< "--leaks --quick --timeout 10000\n";
}

static void print_coverage(bool markdown)
{
	if (markdown)
	{
		std::cout << "| Area | Coverage |\n";
		std::cout << "| --- | --- |\n";
		std::cout << "| Mandatory | invalid fd, closed fd, empty file, EOF repeats, stdin, pipe, blank lines, newline-only files, long lines, buffer edges, read-error handling, optional malloc failure |\n";
		std::cout << "| Bonus | invalid fd, interleaved fds, blank lines, wide round robin, configurable fd limit, independent EOF |\n";
		std::cout << "| Stress | 10k and 100k line fixtures when enabled |\n";
		std::cout << "| Diagnostics | headers, header compile probes, Makefile shape, optional Norminette, JSON and Web doctor reports |\n";
		std::cout << "| Runtime | compile failures, test failures, timeouts, optional Valgrind categories, compact output, profiles, rerun-failed |\n";
		return ;
	}
	std::cout << "Coverage\n";
	std::cout << "--------\n";
	std::cout << "Mandatory: invalid fd, closed fd, empty file, EOF repeats, stdin, pipe,\n";
	std::cout << "           blank lines, newline-only files, long lines, buffer edges,\n";
	std::cout << "           read-error handling, optional malloc failure\n";
	std::cout << "Bonus:     invalid fd, interleaved fds, blank lines, wide round robin,\n";
	std::cout << "           configurable fd limit, independent EOF\n";
	std::cout << "Stress:    optional 10k and 100k line fixtures\n";
	std::cout << "Diagnose:  headers, header probes, Makefile shape, optional Norminette,\n";
	std::cout << "           JSON and Web doctor reports\n";
	std::cout << "Runtime:   compile failures, test failures, timeouts, Valgrind categories,\n";
	std::cout << "           compact output, profiles, rerun-failed\n";
}

static void print_list(void)
{
	std::cout << "Suites\n";
	std::cout << "  mandatory\n";
	std::cout << "  bonus\n";
	std::cout << "\nCases\n";
	for (const CaseInfo &info : case_infos())
		std::cout << "  " << info.name << " (" << info.suite << ")\n";
	std::cout << "\nPresets\n";
	std::cout << "  quick normal strict school pedantic review ci web html stress leaks\n";
	std::cout << "\nReports\n";
	std::cout << "  terminal json web doctor-json doctor-web\n";
}

static void print_cases(void)
{
	std::cout << "Cases\n";
	std::cout << "-----\n";
	for (const CaseInfo &info : case_infos())
		std::cout << std::left << std::setw(17) << info.name
			<< std::setw(16) << info.suite << info.description << "\n";
}

static void print_explain(const std::string &name)
{
	std::string key = name.empty() ? "all" : name;

	if (key == "stdin")
		std::cout << "stdin: redirects a fixture into fd 0 and reads with get_next_line(0).\nCommon failures: rejecting fd 0 or treating stdin differently.\n";
	else if (key == "pipe")
		std::cout << "pipe: reads from a pipe fd instead of a regular file.\nCommon failures: assuming file-specific behavior.\n";
	else if (key == "buffer-edge" || key == "boundary")
		std::cout << "buffer-edge: generated lines at BUFFER_SIZE - 1, BUFFER_SIZE, and BUFFER_SIZE + 1.\nCommon failures: skipped bytes, duplicated bytes, or bad stash handoff.\n";
	else if (key == "bonus" || key == "bonus-many-fds")
		std::cout << "bonus: checks independent state across interleaved file descriptors.\nCommon failures: one global stash, wrong fd index, or early fd cleanup.\n";
	else if (key == "newline-only" || key == "many-newlines")
		std::cout << key << ": checks files made only of newline characters.\nCommon failures: treating blank lines as EOF or dropping newline bytes.\n";
	else if (key == "double-buffer")
		std::cout << "double-buffer: checks lines around BUFFER_SIZE * 2 boundaries.\nCommon failures: off-by-one joins or stale stash content after exact reads.\n";
	else if (key == "read-zero")
		std::cout << "read-zero: checks invalid read paths and repeated invalid fd calls.\nCommon failures: keeping stale stash after read errors.\n";
	else if (key == "malloc-fail")
		std::cout << "malloc-fail: uses a linker malloc wrapper when --malloc-fail is enabled.\nCommon failures: not returning NULL cleanly after allocation failure.\n";
	else if (key == "bonus-wide-fds")
		std::cout << "bonus-wide-fds: checks many simultaneous file descriptors.\nCommon failures: too-small fd storage or shared fd state.\n";
	else if (key == "timeout")
		std::cout << "timeout: kills runs that exceed the configured timeout.\nCommon failures: infinite loop after read returns 0 or -1.\n";
	else if (key == "header")
		std::cout << "header: checks include guard or pragma once plus char *get_next_line(int fd);.\nCommon failures: missing prototype or incompatible declaration.\n";
	else
	{
		for (const CaseInfo &info : case_infos())
		{
			if (key == info.name)
			{
				std::cout << info.name << ": " << info.description << "\n";
				std::cout << "Use --only " << info.name
					<< " for a focused harness run.\n";
				return ;
			}
		}
		std::cout << "Unknown case: " << key << "\n";
		std::cout << "Run ./gnl_tester --cases to list valid cases.\n";
	}
}

static void print_help(void)
{
	std::cout
		<< "Get Next Line Tester\n\n"
		<< "Usage: ./gnl_tester --root /path/to/Get_Next_Line [options]\n\n"
		<< "Options:\n"
		<< "  --version        show tester version\n"
		<< "  --doctor         check tools and target project layout\n"
		<< "  --diagnose       check target Makefile, headers, sources, and warnings\n"
		<< "  --health         print a compact target health summary\n"
		<< "  --norm           include Norminette in diagnostics when available\n"
		<< "  --explain CASE   explain a tested case and common failures\n"
		<< "  --cases          list valid --only and --skip case names\n"
		<< "  --export-fixture CASE  write representative case fixture files\n"
		<< "  --rerun-failed FILE    rerun failed buffers from a JSON report\n"
		<< "  --list           list suites, presets, and report types\n"
		<< "  --coverage       show tested behavior coverage\n"
		<< "  --coverage-md    print tested behavior coverage as Markdown\n"
		<< "  --presets        list named run presets\n"
		<< "  --preset NAME    use quick, normal, strict, school, pedantic, review, ci, web, html, stress, or leaks\n"
		<< "  --root PATH       target project root (default: ..)\n"
		<< "  --compare PATH   compare another target root with the same options\n"
		<< "  --bonus          test get_next_line_bonus files\n"
		<< "  --review         run mandatory strict and bonus strict when available\n"
		<< "  --stress         enable large-line stress fixtures\n"
		<< "  --buffer LIST    comma-separated BUFFER_SIZE values\n"
		<< "  --fd-limit N     fd count for bonus-wide-fds (default: 32)\n"
		<< "  --malloc-fail    enable focused malloc failure harness checks\n"
		<< "  --only CASE      run only a named harness case\n"
		<< "  --skip CASE      skip a named harness case\n"
		<< "  --quick          use BUFFER_SIZE=1,42\n"
		<< "  --strict         use a wider BUFFER_SIZE matrix\n"
		<< "  --leaks          run each suite with valgrind when available\n"
		<< "  --profile        print slowest buffer suites after terminal runs\n"
		<< "  --compact        print a one-line PASS/FAIL summary\n"
		<< "  --summary-only   print only compact suite summaries\n"
		<< "  --fail-fast      stop after the first failing buffer suite\n"
		<< "  --json           print machine-readable JSON output\n"
		<< "  --web, --html    print a standalone Web dashboard report\n"
		<< "  --output FILE    write JSON or Web output to FILE\n"
		<< "  --keep-build     keep per-run build directories after the run\n"
		<< "  --timeout MS     kill a test run after this many ms (default: 3000)\n"
		<< "  --no-color       disable colors\n"
		<< "  --help           show this help\n";
}

static Config parse_args(int argc, char **argv)
{
	Config cfg;

	load_config_file(cfg);
	for (int i = 1; i < argc; ++i)
	{
		std::string arg(argv[i]);
		if (arg == "--version")
			cfg.version = true;
		else if (arg == "--doctor")
			cfg.doctor = true;
		else if (arg == "--diagnose")
			cfg.diagnose = true;
		else if (arg == "--health")
			cfg.health = true;
		else if (arg == "--norm")
			cfg.norm = true;
		else if (arg == "--explain" && i + 1 < argc)
			cfg.explain_case = argv[++i];
		else if (arg.rfind("--explain=", 0) == 0)
			cfg.explain_case = arg.substr(10);
		else if (arg == "--export-fixture" && i + 1 < argc)
			cfg.export_fixture = argv[++i];
		else if (arg.rfind("--export-fixture=", 0) == 0)
			cfg.export_fixture = arg.substr(17);
		else if (arg == "--rerun-failed" && i + 1 < argc)
			cfg.rerun_failed = argv[++i];
		else if (arg.rfind("--rerun-failed=", 0) == 0)
			cfg.rerun_failed = arg.substr(15);
		else if (arg == "--list")
			cfg.list = true;
		else if (arg == "--cases")
			cfg.cases = true;
		else if (arg == "--coverage")
			cfg.coverage = true;
		else if (arg == "--coverage-md")
			cfg.coverage_md = true;
		else if (arg == "--presets")
			cfg.presets = true;
		else if (arg == "--preset" && i + 1 < argc)
		{
			std::string name(argv[++i]);
			if (valid_preset(name))
				apply_preset(cfg, name);
			else
				cfg.invalid_preset = name;
		}
		else if (arg.rfind("--preset=", 0) == 0)
		{
			std::string name = arg.substr(9);
			if (valid_preset(name))
				apply_preset(cfg, name);
			else
				cfg.invalid_preset = name;
		}
		else if (arg == "--root" && i + 1 < argc)
			cfg.root = argv[++i];
		else if (arg.rfind("--root=", 0) == 0)
			cfg.root = arg.substr(7);
		else if (arg == "--compare" && i + 1 < argc)
			cfg.compare_root = argv[++i];
		else if (arg.rfind("--compare=", 0) == 0)
			cfg.compare_root = arg.substr(10);
		else if (arg == "--bonus")
			cfg.bonus = true;
		else if (arg == "--review")
			cfg.review = true;
		else if (arg == "--stress")
			cfg.stress = true;
		else if (arg == "--leaks")
			cfg.leaks = true;
		else if (arg == "--profile")
			cfg.profile = true;
		else if (arg == "--compact")
			cfg.compact = true;
		else if (arg == "--malloc-fail")
			cfg.malloc_fail = true;
		else if (arg == "--summary-only")
			cfg.summary_only = true;
		else if (arg == "--fail-fast")
			cfg.fail_fast = true;
		else if (arg == "--json")
			cfg.json = true;
		else if (arg == "--web" || arg == "--html")
			cfg.web = true;
		else if (arg == "--output" && i + 1 < argc)
			cfg.output = argv[++i];
		else if (arg.rfind("--output=", 0) == 0)
			cfg.output = arg.substr(9);
		else if (arg == "--keep-build")
			cfg.keep_build = true;
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
		else if (arg == "--fd-limit" && i + 1 < argc)
			cfg.fd_limit = std::max(1, std::atoi(argv[++i]));
		else if (arg.rfind("--fd-limit=", 0) == 0)
			cfg.fd_limit = std::max(1, std::atoi(arg.substr(11).c_str()));
		else if (arg == "--only" && i + 1 < argc)
			cfg.only_case = argv[++i];
		else if (arg.rfind("--only=", 0) == 0)
			cfg.only_case = arg.substr(7);
		else if (arg == "--skip" && i + 1 < argc)
			cfg.skip_case = argv[++i];
		else if (arg.rfind("--skip=", 0) == 0)
			cfg.skip_case = arg.substr(7);
		else if (arg == "--help" || arg == "-h")
			cfg.help = true;
	}
	if (cfg.buffers.empty())
		cfg.buffers = {42};
	if (!cfg.only_case.empty() && !valid_case(cfg.only_case))
		cfg.invalid_case = cfg.only_case;
	if (!cfg.skip_case.empty() && !valid_case(cfg.skip_case))
		cfg.invalid_case = cfg.skip_case;
	if (!cfg.export_fixture.empty() && !valid_case(cfg.export_fixture))
		cfg.invalid_case = cfg.export_fixture;
	if (cfg.only_case == "malloc-fail")
		cfg.malloc_fail = true;
	if (cfg.json)
	{
		cfg.color = false;
		cfg.summary_only = true;
	}
	if (cfg.web)
	{
		cfg.color = false;
		cfg.summary_only = true;
	}
	return (cfg);
}

static std::string json_escape(const std::string &value)
{
	std::ostringstream out;

	for (unsigned char c : value)
	{
		if (c == '"')
			out << "\\\"";
		else if (c == '\\')
			out << "\\\\";
		else if (c == '\n')
			out << "\\n";
		else if (c == '\r')
			out << "\\r";
		else if (c == '\t')
			out << "\\t";
		else if (c < 32)
		{
			const char *hex = "0123456789abcdef";
			out << "\\u00" << hex[c >> 4] << hex[c & 15];
		}
		else
			out << c;
	}
	return (out.str());
}

static void print_json_string(std::ostream &out, const std::string &value)
{
	out << "\"" << json_escape(value) << "\"";
}

static void print_json_string_array(std::ostream &out,
	const std::vector<std::string> &values)
{
	out << "[";
	for (size_t i = 0; i < values.size(); ++i)
	{
		if (i > 0)
			out << ",";
		print_json_string(out, values[i]);
	}
	out << "]";
}

static void print_json_buffer_array(std::ostream &out,
	const std::vector<int> &values)
{
	out << "[";
	for (size_t i = 0; i < values.size(); ++i)
	{
		if (i > 0)
			out << ",";
		out << values[i];
	}
	out << "]";
}

static std::string html_escape(const std::string &value)
{
	std::ostringstream out;

	for (unsigned char c : value)
	{
		if (c == '&')
			out << "&amp;";
		else if (c == '<')
			out << "&lt;";
		else if (c == '>')
			out << "&gt;";
		else if (c == '"')
			out << "&quot;";
		else if (c == '\'')
			out << "&#39;";
		else
			out << c;
	}
	return (out.str());
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

static fs::path harness_path(const fs::path &tester_dir, bool bonus);
static fs::path harness_utils_path(const fs::path &tester_dir);
static bool command_exists(const std::string &name);

static bool has_mandatory_files(const fs::path &root)
{
	return (fs::exists(root / "get_next_line.c")
		&& fs::exists(root / "get_next_line_utils.c")
		&& fs::exists(root / "get_next_line.h"));
}

static bool has_include_guard(const std::string &header)
{
	return (header.find("#pragma once") != std::string::npos
		|| (header.find("#ifndef") != std::string::npos
			&& header.find("# define") != std::string::npos
			&& header.find("#endif") != std::string::npos));
}

static std::string compact_spaces(const std::string &value)
{
	std::string out;
	bool space = false;

	for (unsigned char c : value)
	{
		if (std::isspace(c))
			space = true;
		else
		{
			if (space && !out.empty())
				out += ' ';
			out += static_cast<char>(c);
			space = false;
		}
	}
	return (out);
}

static bool has_gnl_prototype(const std::string &header)
{
	std::string compact = compact_spaces(header);

	return (compact.find("char *get_next_line(int fd);") != std::string::npos
		|| compact.find("char* get_next_line(int fd);") != std::string::npos
		|| compact.find("char * get_next_line(int fd);") != std::string::npos);
}

static bool header_integrity(const fs::path &path, std::string &detail)
{
	std::string header;

	if (!fs::exists(path))
	{
		detail = "missing";
		return (false);
	}
	header = read_file(path);
	if (!has_include_guard(header))
	{
		detail = "missing include guard or pragma once";
		return (false);
	}
	if (!has_gnl_prototype(header))
	{
		detail = "missing prototype: char *get_next_line(int fd);";
		return (false);
	}
	detail = "include guard and get_next_line prototype found";
	return (true);
}

static bool makefile_integrity(const fs::path &root, bool bonus,
	std::string &detail)
{
	fs::path path = root / "Makefile";
	std::string text;
	bool has_sources;
	bool has_target;
	bool has_required_targets;
	bool has_bonus_target = true;

	if (!fs::exists(path))
	{
		detail = "missing; optional because this tester compiles sources directly";
		return (true);
	}
	text = read_file(path);
	has_target = (text.find("all:") != std::string::npos
		|| text.find("$(NAME)") != std::string::npos
		|| text.find("${NAME}") != std::string::npos);
	has_required_targets = (text.find("all:") != std::string::npos
		&& text.find("clean:") != std::string::npos
		&& text.find("fclean:") != std::string::npos
		&& text.find("re:") != std::string::npos);
	if (bonus)
		has_bonus_target = (text.find("bonus:") != std::string::npos
			|| text.find("get_next_line_bonus.c") != std::string::npos);
	if (bonus)
		has_sources = (text.find("get_next_line_bonus.c") != std::string::npos
			&& text.find("get_next_line_utils_bonus.c") != std::string::npos);
	else
		has_sources = (text.find("get_next_line.c") != std::string::npos
			&& text.find("get_next_line_utils.c") != std::string::npos);
	if (!has_target || !has_sources || !has_required_targets
		|| !has_bonus_target)
	{
		detail = "found, but expected targets, bonus target, or source names are missing";
		return (false);
	}
	detail = "found with all, clean, fclean, re, and expected source names";
	return (true);
}

static bool header_compile_probe(const Config &cfg, const fs::path &tester_dir,
	const fs::path &header, std::string &detail)
{
	fs::path dir = tester_dir / "tester" / "build" / run_id("probe");
	fs::path probe = dir / "header_probe.c";
	fs::path exe = dir / "header_probe";
	fs::path log = dir / "header_probe.log";
	std::ofstream out;
	std::ostringstream cmd;
	int code;

	fs::create_directories(dir);
	out.open(probe);
	out << "#include \"" << header.filename().string() << "\"\n";
	out << "int main(void) { char *(*fn)(int) = get_next_line; return (fn == 0); }\n";
	out.close();
	cmd << "cc -Wall -Wextra -Werror -I " << quote(cfg.root.string()) << " "
		<< " -c " << quote(probe.string()) << " -o "
		<< quote(exe.string() + ".o");
	code = run_command(cmd.str(), log);
	if (!cfg.keep_build)
		fs::remove_all(dir);
	if (code != 0)
	{
		detail = "header does not compile cleanly against get_next_line prototype";
		return (false);
	}
	detail = "header compiles with get_next_line function pointer probe";
	return (true);
}

static int count_norm_warnings(const fs::path &path, std::ostream &out,
	bool emit)
{
	std::string text;
	int warnings = 0;

	if (!fs::exists(path))
		return (0);
	text = read_file(path);
	if (text.find("int main(") != std::string::npos
		|| text.find("int\tmain(") != std::string::npos)
	{
		if (emit)
			out << "WARN " << path.filename().string()
				<< " contains a main function\n";
		warnings++;
	}
	if (text.find("#define BUFFER_SIZE") != std::string::npos
		|| text.find("# define BUFFER_SIZE") != std::string::npos)
	{
		if (emit)
			out << "WARN " << path.filename().string()
				<< " defines BUFFER_SIZE directly\n";
		warnings++;
	}
	return (warnings);
}

static bool run_norminette(const Config &cfg, const fs::path &tester_dir,
	std::string &detail)
{
	fs::path dir = tester_dir / "tester" / "build" / run_id("norm");
	fs::path log = dir / "norminette.log";
	std::ostringstream cmd;
	int code;

	if (!command_exists("norminette"))
	{
		detail = "norminette not found";
		return (true);
	}
	fs::create_directories(dir);
	cmd << "norminette " << quote((cfg.root / "get_next_line.c").string())
		<< " " << quote((cfg.root / "get_next_line_utils.c").string())
		<< " " << quote((cfg.root / "get_next_line.h").string());
	if (has_bonus_files(cfg.root))
		cmd << " " << quote((cfg.root / "get_next_line_bonus.c").string())
			<< " " << quote((cfg.root / "get_next_line_utils_bonus.c").string())
			<< " " << quote((cfg.root / "get_next_line_bonus.h").string());
	code = run_command(cmd.str(), log);
	detail = read_file(log);
	if (!cfg.keep_build)
		fs::remove_all(dir);
	if (code == 0)
	{
		detail = "norminette passed";
		return (true);
	}
	if (detail.empty())
		detail = "norminette failed without output";
	return (false);
}

static void print_doctor_line(const std::string &label, bool ok,
	const std::string &detail);

static void print_doctor_warn(const std::string &label,
	const std::string &detail);

static void print_doctor_html(std::ostream &out, const Config &cfg,
	const DiagnosticSummary &diag, bool tools_ok, bool tester_ok);

static DiagnosticSummary collect_diagnostics(Config cfg,
	const fs::path &tester_dir, bool emit_warnings)
{
	DiagnosticSummary diag;
	std::ostringstream discard;
	std::ostream &warn_out = emit_warnings ? std::cout : discard;

	diag.mandatory_ok = has_mandatory_files(cfg.root);
	diag.bonus_ok = has_bonus_files(cfg.root);
	diag.makefile_present = fs::exists(cfg.root / "Makefile");
	diag.mandatory_header_ok = header_integrity(cfg.root / "get_next_line.h",
		diag.mandatory_header_detail);
	if (diag.mandatory_header_ok)
		diag.mandatory_probe_ok = header_compile_probe(cfg, tester_dir,
			cfg.root / "get_next_line.h", diag.mandatory_probe_detail);
	else
		diag.mandatory_probe_detail = "skipped because mandatory header is invalid";
	if (diag.bonus_ok)
	{
		diag.bonus_header_ok = header_integrity(cfg.root / "get_next_line_bonus.h",
			diag.bonus_header_detail);
		if (diag.bonus_header_ok)
			diag.bonus_probe_ok = header_compile_probe(cfg, tester_dir,
				cfg.root / "get_next_line_bonus.h", diag.bonus_probe_detail);
		else
			diag.bonus_probe_detail = "skipped because bonus header is invalid";
	}
	diag.makefile_ok = makefile_integrity(cfg.root, cfg.bonus,
		diag.makefile_detail);
	diag.warnings += count_norm_warnings(cfg.root / "get_next_line.c",
		warn_out, emit_warnings);
	diag.warnings += count_norm_warnings(cfg.root / "get_next_line_utils.c",
		warn_out, emit_warnings);
	if (diag.bonus_ok)
	{
		diag.warnings += count_norm_warnings(cfg.root / "get_next_line_bonus.c",
			warn_out, emit_warnings);
		diag.warnings += count_norm_warnings(
			cfg.root / "get_next_line_utils_bonus.c", warn_out, emit_warnings);
	}
	if (cfg.norm)
	{
		diag.norm_checked = command_exists("norminette");
		diag.norm_ok = run_norminette(cfg, tester_dir, diag.norm_detail);
	}
	return (diag);
}

static bool diagnostics_ok(const DiagnosticSummary &diag)
{
	return (diag.mandatory_ok && diag.mandatory_header_ok
		&& diag.mandatory_probe_ok && diag.bonus_header_ok
		&& diag.bonus_probe_ok && diag.makefile_ok && diag.norm_ok);
}

static std::string makefile_status(const DiagnosticSummary &diag)
{
	if (!diag.makefile_present)
		return ("WARN");
	return (diag.makefile_ok ? "OK" : "NOK");
}

static void print_diagnostics_json(std::ostream &out,
	const DiagnosticSummary &diag)
{
	out << "{";
	out << "\"ok\":" << (diagnostics_ok(diag) ? "true" : "false");
	out << ",\"mandatory_files\":" << (diag.mandatory_ok ? "true" : "false");
	out << ",\"bonus_files\":" << (diag.bonus_ok ? "true" : "false");
	out << ",\"mandatory_header\":{";
	out << "\"ok\":" << (diag.mandatory_header_ok ? "true" : "false");
	out << ",\"detail\":";
	print_json_string(out, diag.mandatory_header_detail);
	out << "},\"mandatory_probe\":{";
	out << "\"ok\":" << (diag.mandatory_probe_ok ? "true" : "false");
	out << ",\"detail\":";
	print_json_string(out, diag.mandatory_probe_detail);
	out << "},\"bonus_header\":{";
	out << "\"ok\":" << (diag.bonus_header_ok ? "true" : "false");
	out << ",\"detail\":";
	print_json_string(out, diag.bonus_header_detail);
	out << "},\"bonus_probe\":{";
	out << "\"ok\":" << (diag.bonus_probe_ok ? "true" : "false");
	out << ",\"detail\":";
	print_json_string(out, diag.bonus_probe_detail);
	out << "},\"makefile\":{";
	out << "\"present\":" << (diag.makefile_present ? "true" : "false");
	out << ",\"ok\":" << (diag.makefile_ok ? "true" : "false");
	out << ",\"status\":";
	print_json_string(out, makefile_status(diag));
	out << ",\"detail\":";
	print_json_string(out, diag.makefile_detail);
	out << "},\"warnings\":" << diag.warnings;
	out << ",\"norm\":{";
	out << "\"requested\":" << (!diag.norm_detail.empty() ? "true" : "false");
	out << ",\"checked\":" << (diag.norm_checked ? "true" : "false");
	out << ",\"ok\":" << (diag.norm_ok ? "true" : "false");
	out << ",\"detail\":";
	print_json_string(out, diag.norm_detail);
	out << "}}";
}

static int run_diagnose(Config cfg, const fs::path &tester_dir,
	bool compact)
{
	DiagnosticSummary diag;

	diag = collect_diagnostics(cfg, tester_dir, !compact);
	if (compact)
	{
		std::cout << "health: mandatory " << (diag.mandatory_ok ? "OK" : "NOK")
			<< "  headers "
			<< ((diag.mandatory_header_ok && diag.bonus_header_ok) ? "OK" : "NOK")
			<< "  probes "
			<< ((diag.mandatory_probe_ok && diag.bonus_probe_ok) ? "OK" : "NOK")
			<< "  Makefile " << makefile_status(diag)
			<< "  bonus " << (diag.bonus_ok ? "OK" : "SKIP") << "\n";
		return (diagnostics_ok(diag) ? 0 : 1);
	}
	std::cout << "Get Next Line Tester Diagnose\n\n";
	std::cout << "target: " << cfg.root << "\n\n";
	print_doctor_line("mandatory files", diag.mandatory_ok,
		"get_next_line.c, get_next_line_utils.c, get_next_line.h");
	print_doctor_line("mandatory header", diag.mandatory_header_ok,
		diag.mandatory_header_detail);
	print_doctor_line("header probe", diag.mandatory_probe_ok,
		diag.mandatory_probe_detail);
	print_doctor_line("bonus files", diag.bonus_ok,
		"bonus suite will run automatically in review mode when present");
	if (diag.bonus_ok)
	{
		print_doctor_line("bonus header", diag.bonus_header_ok,
			diag.bonus_header_detail);
		print_doctor_line("bonus probe", diag.bonus_probe_ok,
			diag.bonus_probe_detail);
	}
	if (diag.makefile_present)
		print_doctor_line("Makefile", diag.makefile_ok, diag.makefile_detail);
	else
		print_doctor_warn("Makefile", diag.makefile_detail);
	if (cfg.norm)
	{
		if (diag.norm_checked)
			print_doctor_line("Norminette", diag.norm_ok, diag.norm_detail);
		else
			print_doctor_warn("Norminette", diag.norm_detail);
	}
	std::cout << "\nSummary: " << diag.warnings << " warning(s)\n";
	return (diagnostics_ok(diag) ? 0 : 1);
}

static void print_doctor_line(const std::string &label, bool ok,
	const std::string &detail)
{
	std::cout << (ok ? "OK  " : "NOK ") << std::left << std::setw(18)
		<< label << detail << "\n";
}

static void print_doctor_warn(const std::string &label,
	const std::string &detail)
{
	std::cout << "WARN " << std::left << std::setw(18)
		<< label << detail << "\n";
}

static int run_doctor(Config cfg, const fs::path &tester_dir)
{
	bool tools_ok;
	bool tester_ok;
	DiagnosticSummary diag;
	int diagnose_status;

	tools_ok = command_exists("cc") && command_exists("c++")
		&& command_exists("make");
	tester_ok = fs::exists(harness_path(tester_dir, false))
		&& fs::exists(harness_utils_path(tester_dir));
	if (cfg.json)
	{
		diag = collect_diagnostics(cfg, tester_dir, false);
		std::cout << "{";
		std::cout << "\"tester\":\"Get Next Line Tester\"";
		std::cout << ",\"version\":\"" << g_version << "\"";
		std::cout << ",\"doctor\":true";
		std::cout << ",\"target\":";
		print_json_string(std::cout, cfg.root.string());
		std::cout << ",\"tools\":{";
		std::cout << "\"cc\":" << (command_exists("cc") ? "true" : "false");
		std::cout << ",\"cxx\":" << (command_exists("c++") ? "true" : "false");
		std::cout << ",\"make\":" << (command_exists("make") ? "true" : "false");
		std::cout << ",\"valgrind\":"
			<< (command_exists("valgrind") ? "true" : "false");
		std::cout << "},\"tester_files\":"
			<< (tester_ok ? "true" : "false");
		std::cout << ",\"diagnostics\":";
		print_diagnostics_json(std::cout, diag);
		std::cout << ",\"success\":"
			<< ((tools_ok && tester_ok && diagnostics_ok(diag))
				? "true" : "false");
		std::cout << "}\n";
		return ((tools_ok && tester_ok && diagnostics_ok(diag)) ? 0 : 1);
	}
	if (cfg.web)
	{
		diag = collect_diagnostics(cfg, tester_dir, false);
		print_doctor_html(std::cout, cfg, diag, tools_ok, tester_ok);
		return ((tools_ok && tester_ok && diagnostics_ok(diag)) ? 0 : 1);
	}
	std::cout << "Get Next Line Tester Doctor\n\n";
	std::cout << "target: " << cfg.root << "\n\n";
	print_doctor_line("cc", command_exists("cc"), "target compiler");
	print_doctor_line("c++", command_exists("c++"), "tester compiler");
	print_doctor_line("make", command_exists("make"), "build helper");
	print_doctor_line("valgrind", command_exists("valgrind"),
		"optional leak checker");
	print_doctor_line("tester files", tester_ok, "local harness sources");
	std::cout << "\n";
	diagnose_status = run_diagnose(cfg, tester_dir, false);
	std::cout << "\nRecommended next command:\n";
	if (!tools_ok || !tester_ok || diagnose_status != 0)
		std::cout << "  Fix the NOK items above, then run --doctor again.\n";
	else if (has_bonus_files(cfg.root))
		std::cout << "  ./gnl_tester --root " << quote(cfg.root.string())
			<< " --preset review\n";
	else
		std::cout << "  ./gnl_tester --root " << quote(cfg.root.string())
			<< " --strict\n";
	return ((tools_ok && tester_ok && diagnose_status == 0) ? 0 : 1);
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
	if (cfg.malloc_fail)
		cmd << " -D GNL_MALLOC_FAIL=1 ";
	if (cfg.bonus)
		cmd << quote((cfg.root / "get_next_line_bonus.c").string()) << " "
			<< quote((cfg.root / "get_next_line_utils_bonus.c").string()) << " ";
	else
		cmd << quote((cfg.root / "get_next_line.c").string()) << " "
			<< quote((cfg.root / "get_next_line_utils.c").string()) << " ";
	if (cfg.malloc_fail)
		cmd << " -Wl,--wrap=malloc ";
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

static bool run_ok(const RunResult &res)
{
	return (res.compile_ok && res.tests_ok && res.leaks_ok);
}

static std::string result_label(const RunResult &res);
static std::string likely_fix(const RunResult &res);
static std::vector<std::string> failed_cases_from_output(
	const std::string &output);

static void print_run_result_json(std::ostream &out, const RunResult &res,
	bool include_output)
{
	out << "{";
	out << "\"buffer\":" << res.buffer;
	out << ",\"ok\":" << (run_ok(res) ? "true" : "false");
	out << ",\"compile_ok\":" << (res.compile_ok ? "true" : "false");
	out << ",\"tests_ok\":" << (res.tests_ok ? "true" : "false");
	out << ",\"leaks_ok\":" << (res.leaks_ok ? "true" : "false");
	out << ",\"leak_skipped\":" << (res.leak_skipped ? "true" : "false");
	out << ",\"timed_out\":" << (res.timed_out ? "true" : "false");
	out << ",\"duration_ms\":" << res.duration_ms;
	out << ",\"status\":";
	print_json_string(out, result_label(res));
	out << ",\"likely_fix\":";
	print_json_string(out, likely_fix(res));
	out << ",\"failed_cases\":";
	print_json_string_array(out, failed_cases_from_output(res.test_output));
	out << ",\"leak_issues\":";
	print_json_string_array(out, res.leak_issues);
	if (include_output && !run_ok(res))
	{
		out << ",\"compile_output\":";
		print_json_string(out, res.compile_output);
		out << ",\"test_output\":";
		print_json_string(out, res.test_output);
		out << ",\"leak_output\":";
		print_json_string(out, res.leak_output);
	}
	out << "}";
}

static void print_results_json(std::ostream &out,
	const std::vector<RunResult> &results, bool include_output)
{
	out << "[";
	for (size_t i = 0; i < results.size(); ++i)
	{
		if (i > 0)
			out << ",";
		print_run_result_json(out, results[i], include_output);
	}
	out << "]";
}

static void print_suite_json(std::ostream &out, const SuiteSummary &summary,
	bool include_output)
{
	out << "{";
	out << "\"name\":";
	print_json_string(out, summary.name);
	out << ",\"skipped\":" << (summary.skipped ? "true" : "false");
	out << ",\"success\":" << (summary.success ? "true" : "false");
	out << ",\"passed\":" << summary.passed;
	out << ",\"total\":" << summary.total;
	out << ",\"duration_ms\":" << summary.duration_ms;
	out << ",\"leak_checked\":"
		<< (summary.leak_checked ? "true" : "false");
	out << ",\"leak_skipped\":"
		<< (summary.leak_skipped ? "true" : "false");
	out << ",\"leak_issues\":";
	print_json_string_array(out, summary.leak_issues);
	out << ",\"results\":";
	print_results_json(out, summary.results, include_output);
	out << "}";
}

static std::string result_label(const RunResult &res)
{
	if (run_ok(res))
		return ("PASS");
	if (!res.compile_ok)
		return ("COMPILE");
	if (res.timed_out)
		return ("TIMEOUT");
	if (!res.tests_ok)
		return ("TEST");
	if (!res.leaks_ok)
		return ("LEAKS");
	return ("FAIL");
}

static std::string likely_fix(const RunResult &res)
{
	if (run_ok(res))
		return ("");
	if (!res.compile_ok)
		return ("Check header prototypes, required includes, source names, and -Wall -Wextra -Werror compilation errors.");
	if (res.timed_out)
		return ("Check infinite loops, EOF handling, and stash cleanup after read returns 0 or -1.");
	if (!res.tests_ok)
		return ("Focus this case with --only and compare expected/got output. Common causes are dropped newlines, early NULL returns, or stale buffer content.");
	if (!res.leaks_ok)
		return ("Inspect allocations, EOF cleanup, failed read paths, and Valgrind leak categories.");
	return ("Rerun this buffer with --no-color and inspect the generated logs.");
}

static std::vector<std::string> failed_cases_from_output(
	const std::string &output)
{
	std::vector<std::string> cases;
	std::stringstream ss(output);
	std::string line;

	while (std::getline(ss, line))
	{
		size_t start;
		size_t end;

		if (line.rfind("NOK ", 0) != 0)
			continue ;
		start = 4;
		end = line.find(" expected=", start);
		if (end == std::string::npos)
			end = line.find(" got=", start);
		if (end == std::string::npos)
			end = line.size();
		add_unique(cases, line.substr(start, end - start));
	}
	return (cases);
}

static void print_case_filter_json(std::ostream &out, const Config &cfg)
{
	out << "{";
	out << "\"only\":";
	print_json_string(out, cfg.only_case);
	out << ",\"skip\":";
	print_json_string(out, cfg.skip_case);
	out << "}";
}

static std::vector<int> failed_buffers_from_report(const fs::path &path)
{
	std::string text = read_file(path);
	std::vector<int> buffers;
	size_t pos = 0;

	while ((pos = text.find("\"buffer\":", pos)) != std::string::npos)
	{
		size_t value_pos = pos + 9;
		int buffer = std::atoi(text.c_str() + value_pos);
		size_t object_end = text.find("}", value_pos);
		size_t ok_pos = text.find("\"ok\":false", value_pos);

		if (buffer > 0 && ok_pos != std::string::npos
			&& (object_end == std::string::npos || ok_pos < object_end))
			buffers.push_back(buffer);
		pos = value_pos;
	}
	std::sort(buffers.begin(), buffers.end());
	buffers.erase(std::unique(buffers.begin(), buffers.end()), buffers.end());
	return (buffers);
}

static void write_fixture_file(const fs::path &dir, const std::string &name,
	const std::string &content)
{
	std::ofstream out;

	fs::create_directories(dir);
	out.open(dir / name, std::ios::binary);
	if (!out)
		throw std::runtime_error("could not write exported fixture");
	out << content;
}

static std::string repeated(char c, int count)
{
	if (count < 1)
		count = 1;
	return (std::string(static_cast<size_t>(count), c));
}

static int export_fixture(const Config &cfg)
{
	fs::path dir = cfg.output.empty() ? fs::path("gnl-exported-fixtures")
		: cfg.output;
	int buffer = cfg.buffers.empty() ? 42 : cfg.buffers.front();

	try
	{
		if (cfg.export_fixture == "empty")
			write_fixture_file(dir, "empty.txt", "");
		else if (cfg.export_fixture == "one-line")
		{
			write_fixture_file(dir, "one_no_newline.txt", "hello");
			write_fixture_file(dir, "one_newline.txt", "hello\n");
		}
		else if (cfg.export_fixture == "many")
			write_fixture_file(dir, "many.txt", "alpha\nbeta\ngamma");
		else if (cfg.export_fixture == "blank")
			write_fixture_file(dir, "blank.txt", "\n\nabc\n\nlast");
		else if (cfg.export_fixture == "newline-only")
			write_fixture_file(dir, "newline_only.txt", "\n");
		else if (cfg.export_fixture == "many-newlines")
			write_fixture_file(dir, "many_newlines.txt", "\n\n\n\n");
		else if (cfg.export_fixture == "long")
			write_fixture_file(dir, "long.txt", repeated('a', 200) + "\ntail");
		else if (cfg.export_fixture == "buffer-edge")
		{
			write_fixture_file(dir, "buffer_minus_one.txt",
				repeated('b', buffer - 1) + "\ntail");
			write_fixture_file(dir, "buffer_exact.txt",
				repeated('b', buffer) + "\ntail");
			write_fixture_file(dir, "buffer_plus_one.txt",
				repeated('b', buffer + 1) + "\ntail");
		}
		else if (cfg.export_fixture == "double-buffer")
		{
			write_fixture_file(dir, "buffer_double.txt",
				repeated('d', buffer * 2) + "\ntail");
			write_fixture_file(dir, "buffer_double_plus_one.txt",
				repeated('d', buffer * 2 + 1) + "\ntail");
		}
		else if (cfg.export_fixture == "stdin")
			write_fixture_file(dir, "stdin.txt", "stdin-a\nstdin-b\nstdin-last");
		else if (cfg.export_fixture == "pipe")
			write_fixture_file(dir, "pipe.txt", "pipe-a\npipe-b\nlast");
		else if (cfg.export_fixture == "bonus-basic")
		{
			write_fixture_file(dir, "bonus_a.txt", "a1\na2\na3");
			write_fixture_file(dir, "bonus_b.txt", "b1\n\nb3\n");
		}
		else if (cfg.export_fixture == "bonus-many-fds"
			|| cfg.export_fixture == "bonus-wide-fds")
		{
			int count = cfg.export_fixture == "bonus-wide-fds" ? cfg.fd_limit : 8;
			for (int i = 0; i < count; ++i)
				write_fixture_file(dir, "bonus_fd_" + std::to_string(i) + ".txt",
					"fd" + std::to_string(i) + "-a\nfd"
					+ std::to_string(i) + "-b\nfd"
					+ std::to_string(i) + "-last");
		}
		else
			write_fixture_file(dir, cfg.export_fixture + ".txt",
				"Generated case does not require a disk fixture.\n");
	}
	catch (const std::exception &err)
	{
		std::cerr << err.what() << "\n";
		return (2);
	}
	std::cout << "Exported fixture " << cfg.export_fixture << " to "
		<< dir << "\n";
	return (0);
}

static std::string first_failure_output(const RunResult &res)
{
	if (!res.compile_ok)
	{
		if (res.compile_output.empty())
			return ("Compilation failed without output.");
		return (res.compile_output);
	}
	if (res.timed_out)
		return ("Test run exceeded the configured timeout.");
	if (!res.tests_ok)
	{
		if (res.test_output.empty())
			return ("Test binary exited with a failure status but produced no output.");
		return (res.test_output);
	}
	if (!res.leaks_ok)
	{
		if (res.leak_output.empty())
			return ("Valgrind failed without output.");
		return (res.leak_output);
	}
	return ("");
}

static int suite_failed(const SuiteSummary &summary)
{
	return (summary.total - summary.passed);
}

static int pass_percent(int passed, int total)
{
	if (total <= 0)
		return (0);
	return ((passed * 100) / total);
}

static void print_html_head(std::ostream &out)
{
	out << "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">";
	out << "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
	out << "<title>Get Next Line Tester Web Dashboard</title>";
	out << "<style>";
	out << ":root{--bg:#101315;--panel:#f7f2e8;--ink:#191d20;--muted:#687076;";
	out << "--line:#d7d0c2;--dark:#20262a;--ok:#19764a;--bad:#b73a2f;";
	out << "--warn:#a96f00;--accent:#1f6feb}";
	out << "*{box-sizing:border-box}body{margin:0;background:#101315;color:var(--ink);";
	out << "font:15px/1.5 ui-sans-serif,system-ui,-apple-system,BlinkMacSystemFont,";
	out << "\"Segoe UI\",sans-serif}main{max-width:1180px;margin:0 auto;padding:28px}";
	out << ".hero{background:var(--panel);border:1px solid var(--line);border-radius:8px;";
	out << "padding:28px;margin-bottom:18px}.top{display:flex;gap:18px;justify-content:";
	out << "space-between;align-items:flex-start;flex-wrap:wrap}.eyebrow{margin:0 0 8px;";
	out << "font-size:12px;letter-spacing:.12em;text-transform:uppercase;color:var(--muted);";
	out << "font-weight:800}h1{margin:0;font-size:42px;line-height:1.05;letter-spacing:0}";
	out << "h2{margin:0 0 14px;font-size:22px}.verdict{display:inline-flex;align-items:";
	out << "center;justify-content:center;min-width:120px;border-radius:8px;padding:12px 18px;";
	out << "font-weight:900;color:white}.verdict.pass{background:var(--ok)}.verdict.fail{";
	out << "background:var(--bad)}.muted{color:var(--muted)}.grid{display:grid;";
	out << "grid-template-columns:repeat(4,minmax(0,1fr));gap:12px;margin-top:22px}";
	out << ".metric,.panel{background:var(--panel);border:1px solid var(--line);";
	out << "border-radius:8px;padding:18px}.metric span{display:block;color:var(--muted);";
	out << "font-size:12px;text-transform:uppercase;font-weight:800}.metric strong{display:block;";
	out << "font-size:30px;line-height:1.1;margin-top:6px}.bar{height:12px;border:1px solid ";
	out << "var(--line);border-radius:999px;background:#ebe4d8;overflow:hidden}.bar span{";
	out << "display:block;height:100%;background:linear-gradient(90deg,var(--ok),#5fa777)}";
	out << ".panel{margin-top:14px}.toolbar{display:flex;gap:10px;align-items:center;";
	out << "flex-wrap:wrap;margin:10px 0 16px}.filter,.copy{border:1px solid var(--line);";
	out << "background:#fffaf0;border-radius:8px;padding:8px 10px;font:inherit;font-weight:800;";
	out << "cursor:pointer}.filter.active{border-color:var(--accent);color:var(--accent)}";
	out << ".table-wrap{overflow:auto;border:1px solid var(--line);border-radius:8px;background:white}";
	out << "table{width:100%;border-collapse:collapse;min-width:760px}th,td{padding:12px;";
	out << "border-bottom:1px solid var(--line);text-align:left;vertical-align:top}th{font-size:";
	out << "12px;text-transform:uppercase;letter-spacing:.08em;color:var(--muted)}tr:last-child td{";
	out << "border-bottom:0}.pill{display:inline-block;border:1px solid currentColor;border-radius:";
	out << "999px;padding:3px 8px;font-size:12px;font-weight:900}.pass-text{color:var(--ok)}";
	out << ".fail-text{color:var(--bad)}.warn-text{color:var(--warn)}details{border:1px solid ";
	out << "var(--line);border-radius:8px;background:white;margin-top:10px}summary{cursor:pointer;";
	out << "padding:12px;font-weight:900}pre{white-space:pre-wrap;overflow:auto;margin:0;padding:14px;";
	out << "background:#111820;color:#f2f5f7;border-top:1px solid #303a44;border-radius:0 0 8px 8px}";
	out << ".hide{display:none}@media(max-width:800px){main{padding:16px}.grid{grid-template-columns:";
	out << "repeat(2,minmax(0,1fr))}h1{font-size:32px}}";
	out << "</style></head><body><main>";
}

static void print_html_script(std::ostream &out)
{
	out << "<script>";
	out << "function filterRows(mode){document.querySelectorAll('[data-result]').forEach(function(r){";
	out << "r.classList.toggle('hide',mode!=='all'&&r.dataset.result!==mode)});";
	out << "document.querySelectorAll('.filter').forEach(function(b){b.classList.toggle('active',";
	out << "b.dataset.mode===mode)})}";
	out << "function copyText(id){navigator.clipboard&&navigator.clipboard.writeText(";
	out << "document.getElementById(id).innerText)}";
	out << "</script></main></body></html>\n";
}

static void print_html_metric(std::ostream &out, const std::string &label,
	const std::string &value)
{
	out << "<div class=\"metric\"><span>" << html_escape(label)
		<< "</span><strong>" << html_escape(value) << "</strong></div>";
}

static void print_html_result_row(std::ostream &out, const RunResult &res,
	const std::string &suite_name)
{
	std::string label = result_label(res);
	std::string result_class = run_ok(res) ? "passed" : "failed";

	out << "<tr data-result=\"" << result_class << "\"><td>"
		<< html_escape(suite_name) << "</td><td>BUFFER_SIZE=" << res.buffer
		<< "</td><td><span class=\"pill "
		<< (run_ok(res) ? "pass-text" : "fail-text") << "\">"
		<< html_escape(label) << "</span></td><td>"
		<< res.duration_ms << "ms</td><td>";
	if (!run_ok(res))
	{
		out << "<details><summary>Failure output</summary><pre>";
		out << html_escape(first_failure_output(res));
		out << "</pre></details>";
		out << "<details><summary>Likely fix</summary><pre>";
		out << html_escape(likely_fix(res));
		out << "</pre></details>";
	}
	else
		out << "<span class=\"muted\">No issues</span>";
	out << "</td></tr>";
}

static void print_html_suite_panel(std::ostream &out,
	const SuiteSummary &summary)
{
	int percent = pass_percent(summary.passed, summary.total);

	out << "<section class=\"panel\"><h2>" << html_escape(summary.name)
		<< "</h2>";
	if (summary.skipped)
	{
		out << "<p class=\"muted\">Suite skipped.</p></section>";
		return ;
	}
	out << "<p><strong class=\"" << (summary.success ? "pass-text" : "fail-text")
		<< "\">" << summary.passed << "/" << summary.total
		<< "</strong> buffer suites passed in " << summary.duration_ms
		<< "ms.</p>";
	out << "<div class=\"bar\"><span style=\"width:" << percent
		<< "%\"></span></div>";
	if (!summary.leak_issues.empty())
	{
		out << "<p class=\"fail-text\">Valgrind: "
			<< html_escape(join_issues(summary.leak_issues)) << "</p>";
	}
	out << "</section>";
}

static void print_html_diagnostics(std::ostream &out,
	const DiagnosticSummary &diag)
{
	out << "<section class=\"panel\"><h2>Diagnostics</h2><div class=\"grid\">";
	print_html_metric(out, "Mandatory Files",
		diag.mandatory_ok ? "OK" : "NOK");
	print_html_metric(out, "Headers",
		(diag.mandatory_header_ok && diag.bonus_header_ok) ? "OK" : "NOK");
	print_html_metric(out, "Header Probes",
		(diag.mandatory_probe_ok && diag.bonus_probe_ok) ? "OK" : "NOK");
	print_html_metric(out, "Makefile", makefile_status(diag));
	print_html_metric(out, "Warnings", std::to_string(diag.warnings));
	print_html_metric(out, "Bonus Files", diag.bonus_ok ? "OK" : "SKIP");
	if (!diag.norm_detail.empty())
		print_html_metric(out, "Norminette",
			diag.norm_checked ? (diag.norm_ok ? "OK" : "NOK") : "SKIP");
	out << "</div>";
	out << "<details><summary>Diagnostic Details</summary><pre>";
	out << "mandatory header: " << html_escape(diag.mandatory_header_detail)
		<< "\n";
	out << "mandatory probe: " << html_escape(diag.mandatory_probe_detail)
		<< "\n";
	if (diag.bonus_ok)
	{
		out << "bonus header: " << html_escape(diag.bonus_header_detail)
			<< "\n";
		out << "bonus probe: " << html_escape(diag.bonus_probe_detail)
			<< "\n";
	}
	out << "Makefile: " << html_escape(diag.makefile_detail) << "\n";
	if (!diag.norm_detail.empty())
		out << "Norminette: " << html_escape(diag.norm_detail) << "\n";
	out << "</pre></details></section>";
}

static void print_doctor_html(std::ostream &out, const Config &cfg,
	const DiagnosticSummary &diag, bool tools_ok, bool tester_ok)
{
	bool success = tools_ok && tester_ok && diagnostics_ok(diag);

	print_html_head(out);
	out << "<section class=\"hero\"><div class=\"top\"><div>";
	out << "<p class=\"eyebrow\">Get Next Line Tester</p>";
	out << "<h1>Doctor Report</h1>";
	out << "<p class=\"muted\">Target: " << html_escape(cfg.root.string())
		<< "</p>";
	out << "</div><div class=\"verdict " << (success ? "pass" : "fail")
		<< "\">" << (success ? "PASS" : "FAIL") << "</div></div>";
	out << "<div class=\"grid\">";
	print_html_metric(out, "Tools", tools_ok ? "OK" : "NOK");
	print_html_metric(out, "Tester Files", tester_ok ? "OK" : "NOK");
	print_html_metric(out, "Makefile", makefile_status(diag));
	print_html_metric(out, "Warnings", std::to_string(diag.warnings));
	out << "</div></section>";
	print_html_diagnostics(out, diag);
	out << "<section class=\"panel\"><h2>Recommended Command</h2><pre>";
	if (!success)
		out << "Fix the NOK items above, then run ./gnl_tester --doctor again.";
	else if (diag.bonus_ok)
		out << "./gnl_tester --root " << html_escape(quote(cfg.root.string()))
			<< " --preset review";
	else
		out << "./gnl_tester --root " << html_escape(quote(cfg.root.string()))
			<< " --strict";
	out << "</pre></section>";
	print_html_script(out);
}

static std::string html_rerun_command(const Config &cfg,
	const SuiteSummary &suite, const RunResult &res)
{
	std::ostringstream cmd;

	cmd << "./gnl_tester --root " << quote(cfg.root.string());
	if (suite.name == "Bonus")
		cmd << " --bonus";
	cmd << " --buffer " << res.buffer;
	if (cfg.stress)
		cmd << " --stress";
	if (cfg.leaks)
		cmd << " --leaks";
	if (cfg.fail_fast)
		cmd << " --fail-fast";
	if (cfg.timeout_ms != 3000)
		cmd << " --timeout " << cfg.timeout_ms;
	cmd << " --no-color";
	return (cmd.str());
}

static void print_html_report(std::ostream &out, const Config &cfg,
	const fs::path &tester_dir,
	const std::vector<SuiteSummary> &suites, bool review, bool success)
{
	int total = 0;
	int passed = 0;
	int failed = 0;
	int suite_count = 0;
	DiagnosticSummary diag;

	diag = collect_diagnostics(cfg, tester_dir, false);
	for (const SuiteSummary &suite : suites)
	{
		if (suite.skipped)
			continue ;
		total += suite.total;
		passed += suite.passed;
		failed += suite_failed(suite);
		suite_count++;
	}
	print_html_head(out);
	out << "<section class=\"hero\"><div class=\"top\"><div>";
	out << "<p class=\"eyebrow\">Get Next Line Tester</p>";
	out << "<h1>Web Dashboard</h1>";
	out << "<p class=\"muted\">Target: " << html_escape(cfg.root.string())
		<< "</p>";
	out << "</div><div class=\"verdict " << (success ? "pass" : "fail")
		<< "\">" << (success ? "PASS" : "FAIL") << "</div></div>";
	out << "<div class=\"grid\">";
	print_html_metric(out, "Mode", review ? "review" : (cfg.bonus ? "bonus" : "mandatory"));
	print_html_metric(out, "Passed", std::to_string(passed) + "/" + std::to_string(total));
	print_html_metric(out, "Failed", std::to_string(failed));
	print_html_metric(out, "Suites", std::to_string(suite_count));
	out << "</div></section>";
	out << "<section class=\"panel\"><h2>Run Configuration</h2><div class=\"grid\">";
	print_html_metric(out, "Version", g_version);
	print_html_metric(out, "Timeout", std::to_string(cfg.timeout_ms) + "ms");
	print_html_metric(out, "Stress", cfg.stress ? "enabled" : "skipped");
	print_html_metric(out, "Leaks", cfg.leaks ? "enabled" : "skipped");
	print_html_metric(out, "Fail Fast", cfg.fail_fast ? "enabled" : "disabled");
	out << "</div></section>";
	print_html_diagnostics(out, diag);
	for (const SuiteSummary &suite : suites)
		print_html_suite_panel(out, suite);
	if (!cfg.command.empty())
	{
		out << "<section class=\"panel\"><h2>Command</h2><pre>"
			<< html_escape(cfg.command) << "</pre></section>";
	}
	out << "<section class=\"panel\"><h2>Buffer Results</h2>";
	out << "<div class=\"toolbar\">";
	out << "<button class=\"filter active\" data-mode=\"all\" onclick=\"filterRows('all')\">All</button>";
	out << "<button class=\"filter\" data-mode=\"passed\" onclick=\"filterRows('passed')\">Passed</button>";
	out << "<button class=\"filter\" data-mode=\"failed\" onclick=\"filterRows('failed')\">Failed</button>";
	out << "</div><div class=\"table-wrap\"><table><thead><tr>";
	out << "<th>Suite</th><th>Buffer</th><th>Status</th><th>Duration</th><th>Details</th>";
	out << "</tr></thead><tbody>";
	for (const SuiteSummary &suite : suites)
	{
		for (const RunResult &res : suite.results)
			print_html_result_row(out, res, suite.name);
	}
	out << "</tbody></table></div></section>";
	if (failed > 0)
	{
		out << "<section class=\"panel\"><h2>Rerun Commands</h2>";
		out << "<button class=\"copy\" onclick=\"copyText('reruns')\">Copy commands</button>";
		out << "<pre id=\"reruns\">";
		for (const SuiteSummary &suite : suites)
		{
			for (const RunResult &res : suite.results)
			{
				if (!run_ok(res))
				{
					out << html_escape(html_rerun_command(cfg, suite, res))
						<< "\n";
				}
			}
		}
		out << "</pre></section>";
	}
	print_html_script(out);
}

static RunResult run_one(const Config &cfg, const fs::path &build,
	const fs::path &harness, const fs::path &utils, int buffer)
{
	RunResult res;
	auto start = std::chrono::steady_clock::now();
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
	{
		res.duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - start).count();
		return (res);
	}
	code = run_command_timeout({exe.string(), run_dir.string(),
		cfg.only_case, cfg.skip_case, std::to_string(cfg.fd_limit)}, test_log,
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
			run_dir.string(),
			cfg.only_case,
			cfg.skip_case,
			std::to_string(cfg.fd_limit)
		}, leak_log, cfg.timeout_ms, res.timed_out);
		res.leak_output = read_file(leak_log);
		res.leak_issues = classify_valgrind(res.leak_output);
		if (code != 0 && !res.timed_out && res.leak_issues.empty())
			add_unique(res.leak_issues, "valgrind error");
		res.leaks_ok = (code == 0 && !res.timed_out
			&& res.leak_issues.empty());
	}
	res.duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - start).count();
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
	std::cout << " " << res.duration_ms << "ms";
	if (res.timed_out)
		std::cout << " timeout(" << cfg.timeout_ms << "ms)";
	std::cout << "\n";
	if (!res.compile_ok)
		std::cout << first_failure_output(res) << "\n";
	else if (res.timed_out)
		std::cout << "Test run exceeded " << cfg.timeout_ms << "ms.\n";
	else if (!res.tests_ok)
		std::cout << first_failure_output(res) << "\n";
	else if (!res.leaks_ok)
	{
		if (!res.leak_issues.empty())
			std::cout << "Valgrind: NOK " << join_issues(res.leak_issues)
				<< "\n";
		std::cout << first_failure_output(res) << "\n";
	}
}

static void print_profile(const std::vector<RunResult> &results)
{
	std::vector<RunResult> sorted = results;

	std::sort(sorted.begin(), sorted.end(), [](const RunResult &a,
		const RunResult &b) {
		return (a.duration_ms > b.duration_ms);
	});
	std::cout << "Profile\n";
	for (size_t i = 0; i < sorted.size() && i < 5; ++i)
		std::cout << "  BUFFER_SIZE=" << sorted[i].buffer << " "
			<< sorted[i].duration_ms << "ms " << result_label(sorted[i])
			<< "\n";
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
		if (run_ok(res))
			summary.passed++;
		summary.total++;
		summary.duration_ms += res.duration_ms;
		summary.results.push_back(res);
		if (suite_cfg.leaks)
		{
			if (res.leak_skipped)
				summary.leak_skipped = true;
			else if (res.compile_ok && res.tests_ok)
				summary.leak_checked = true;
			for (const std::string &issue : res.leak_issues)
				add_unique(summary.leak_issues, issue);
		}
		if (!suite_cfg.summary_only && !suite_cfg.json
			&& (verbose || !run_ok(res)))
			print_result(suite_cfg, res);
		if (suite_cfg.fail_fast && !run_ok(res))
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
	fs::path build = tester_dir / "tester" / "build" / run_id("review");
	SuiteSummary mandatory;
	SuiteSummary bonus;
	SuiteSummary leak_summary;
	bool bonus_available;
	bool pass;

	cfg.buffers = {1, 2, 3, 4, 5, 7, 8, 16, 32, 42, 64, 128, 1024};
	bonus_available = has_bonus_files(cfg.root);
	fs::create_directories(build);
	if (!cfg.json && !cfg.web && !cfg.compact)
	{
		std::cout << "Get Next Line Tester Review\n\n";
		std::cout << "target:  " << cfg.root << "\n";
		std::cout << "timeout: " << cfg.timeout_ms << "ms\n";
		std::cout << "stress:  " << (cfg.stress ? "enabled" : "skipped") << "\n";
		std::cout << "leaks:   " << (cfg.leaks ? "enabled" : "skipped") << "\n\n";
	}
	mandatory = run_suite(cfg, tester_dir, build, false, false);
	if (bonus_available)
		bonus = run_suite(cfg, tester_dir, build, true, false);
	else
	{
		bonus.name = "Bonus";
		bonus.skipped = true;
	}
	merge_leak_summary(leak_summary, mandatory);
	if (!bonus.skipped)
		merge_leak_summary(leak_summary, bonus);
	pass = mandatory.success && (bonus.skipped || bonus.success);
	if (cfg.json)
	{
		DiagnosticSummary diag = collect_diagnostics(cfg, tester_dir, false);

		std::cout << "{";
		std::cout << "\"tester\":\"Get Next Line Tester\"";
		std::cout << ",\"version\":\"" << g_version << "\"";
		std::cout << ",\"review\":true";
		std::cout << ",\"target\":";
		print_json_string(std::cout, cfg.root.string());
		std::cout << ",\"command\":";
		print_json_string(std::cout, cfg.command);
		std::cout << ",\"timeout_ms\":" << cfg.timeout_ms;
		std::cout << ",\"stress\":" << (cfg.stress ? "true" : "false");
		std::cout << ",\"leaks\":" << (cfg.leaks ? "true" : "false");
		std::cout << ",\"norm\":" << (cfg.norm ? "true" : "false");
		std::cout << ",\"malloc_fail\":"
			<< (cfg.malloc_fail ? "true" : "false");
		std::cout << ",\"fd_limit\":" << cfg.fd_limit;
		std::cout << ",\"success\":" << (pass ? "true" : "false");
		std::cout << ",\"verdict\":\"" << (pass ? "PASS" : "FAIL") << "\"";
		std::cout << ",\"duration_ms\":"
			<< (mandatory.duration_ms + bonus.duration_ms);
		std::cout << ",\"buffers\":";
		print_json_buffer_array(std::cout, cfg.buffers);
		std::cout << ",\"case_filter\":";
		print_case_filter_json(std::cout, cfg);
		std::cout << ",\"diagnostics\":";
		print_diagnostics_json(std::cout, diag);
		std::cout << ",\"suites\":[";
		print_suite_json(std::cout, mandatory, true);
		std::cout << ",";
		print_suite_json(std::cout, bonus, true);
		std::cout << "]";
		std::cout << ",\"valgrind\":{";
		std::cout << "\"enabled\":" << (cfg.leaks ? "true" : "false");
		std::cout << ",\"checked\":"
			<< (leak_summary.leak_checked ? "true" : "false");
		std::cout << ",\"skipped\":"
			<< (leak_summary.leak_skipped ? "true" : "false");
		std::cout << ",\"issues\":";
		print_json_string_array(std::cout, leak_summary.leak_issues);
		std::cout << "}}\n";
	}
	else if (cfg.web)
	{
		std::vector<SuiteSummary> suites;

		suites.push_back(mandatory);
		suites.push_back(bonus);
		print_html_report(std::cout, cfg, tester_dir, suites, true, pass);
	}
	else
	{
		if (cfg.compact)
		{
			std::cout << (pass ? "PASS" : "FAIL") << " mandatory "
				<< mandatory.passed << "/" << mandatory.total << " bonus ";
			if (bonus.skipped)
				std::cout << "SKIP";
			else
				std::cout << bonus.passed << "/" << bonus.total;
			std::cout << "\n";
		}
		else
		{
			std::cout << "\n";
			print_review_line(cfg, mandatory);
			print_review_line(cfg, bonus);
			print_valgrind_review_line(cfg, leak_summary);
			std::cout << "Verdict:  "
				<< (pass ? paint(cfg, "\033[32m") : paint(cfg, "\033[31m"))
				<< (pass ? "PASS" : "FAIL") << paint(cfg, "\033[0m") << "\n";
		}
	}
	if (!cfg.keep_build)
		fs::remove_all(build);
	return (pass ? 0 : 1);
}

static int run_compare(Config cfg, const fs::path &tester_dir)
{
	fs::path build = tester_dir / "tester" / "build" / run_id("compare");
	Config left = cfg;
	Config right = cfg;
	SuiteSummary left_summary;
	SuiteSummary right_summary;
	bool left_ok;
	bool right_ok;

	if (cfg.json || cfg.web)
	{
		std::cerr << "--compare currently supports terminal output only\n";
		return (2);
	}
	left.summary_only = true;
	right.summary_only = true;
	left.root = fs::weakly_canonical(cfg.root);
	right.root = fs::weakly_canonical(cfg.compare_root);
	fs::create_directories(build);
	left_summary = run_suite(left, tester_dir, build / "left", cfg.bonus, false);
	right_summary = run_suite(right, tester_dir, build / "right", cfg.bonus, false);
	left_ok = left_summary.success;
	right_ok = right_summary.success;
	std::cout << "Get Next Line Tester Compare\n\n";
	std::cout << "target:  " << left.root << "\n";
	std::cout << "compare: " << right.root << "\n";
	std::cout << "mode:    " << (cfg.bonus ? "bonus" : "mandatory") << "\n\n";
	std::cout << "Target:  " << (left_ok ? "OK " : "NOK ")
		<< left_summary.passed << "/" << left_summary.total
		<< " in " << left_summary.duration_ms << "ms\n";
	std::cout << "Compare: " << (right_ok ? "OK " : "NOK ")
		<< right_summary.passed << "/" << right_summary.total
		<< " in " << right_summary.duration_ms << "ms\n";
	if (left_summary.passed > right_summary.passed)
		std::cout << "Verdict:  TARGET BETTER\n";
	else if (left_summary.passed < right_summary.passed)
		std::cout << "Verdict:  TARGET WORSE\n";
	else
		std::cout << "Verdict:  SAME SCORE\n";
	if (!cfg.keep_build)
		fs::remove_all(build);
	return (left_ok ? 0 : 1);
}

int main(int argc, char **argv)
{
	Config cfg = parse_args(argc, argv);
	fs::path tester_dir = executable_dir(argv[0]);
	fs::path build = tester_dir / "tester" / "build" / run_id("run");
	fs::path harness;
	fs::path harness_utils;
	std::ofstream output_file;
	std::streambuf *stdout_buffer = std::cout.rdbuf();
	std::vector<RunResult> results;
	SuiteSummary summary;
	int passed = 0;
	int total = 0;
	auto finish = [&](int code) {
		if (!cfg.keep_build)
			fs::remove_all(build);
		std::cout.rdbuf(stdout_buffer);
		return (code);
	};

	cfg.command = command_line(argc, argv);
	if (cfg.version)
	{
		std::cout << "Get Next Line Tester " << g_version << "\n";
		return (0);
	}
	if (!cfg.invalid_preset.empty())
	{
		std::cerr << "Unknown preset: " << cfg.invalid_preset << "\n";
		std::cerr << "Run ./gnl_tester --presets to list available presets.\n";
		return (2);
	}
	if (!cfg.invalid_case.empty())
	{
		std::cerr << "Unknown case: " << cfg.invalid_case << "\n";
		std::cerr << "Run ./gnl_tester --cases to list valid cases.\n";
		return (2);
	}
	if (cfg.help)
	{
		print_help();
		return (0);
	}
	if (cfg.presets)
	{
		print_presets();
		return (0);
	}
	if (cfg.list)
	{
		print_list();
		return (0);
	}
	if (cfg.cases)
	{
		print_cases();
		return (0);
	}
	if (cfg.coverage || cfg.coverage_md)
	{
		print_coverage(cfg.coverage_md);
		return (0);
	}
	if (!cfg.explain_case.empty())
	{
		print_explain(cfg.explain_case);
		return (0);
	}
	cfg.root = fs::weakly_canonical(cfg.root);
	if (!cfg.rerun_failed.empty())
	{
		cfg.buffers = failed_buffers_from_report(cfg.rerun_failed);
		if (cfg.buffers.empty())
		{
			std::cerr << "No failed buffers found in " << cfg.rerun_failed << "\n";
			return (2);
		}
	}
	if (!cfg.export_fixture.empty())
		return (finish(export_fixture(cfg)));
	if (!cfg.output.empty())
	{
		if (!cfg.json && !cfg.web)
		{
			std::cerr << "--output can only be used with --json, --web, or --html\n";
			return (2);
		}
		output_file.open(cfg.output);
		if (!output_file)
		{
			std::cerr << "Could not open output file " << cfg.output << "\n";
			return (2);
		}
		std::cout.rdbuf(output_file.rdbuf());
	}
	if (cfg.health)
		return (finish(run_diagnose(cfg, tester_dir, true)));
	if (cfg.diagnose)
		return (finish(run_diagnose(cfg, tester_dir, false)));
	if (cfg.doctor)
		return (finish(run_doctor(cfg, tester_dir)));
	if (!cfg.compare_root.empty())
		return (finish(run_compare(cfg, tester_dir)));
	if (cfg.review)
	{
		int code = run_review(cfg, tester_dir);
		return (finish(code));
	}
	if (!validate_root(cfg))
		return (finish(2));
	harness = harness_path(tester_dir, cfg.bonus);
	harness_utils = harness_utils_path(tester_dir);
	if (!fs::exists(harness))
	{
		std::cerr << "Missing " << harness << "\n";
		return (finish(2));
	}
	if (!fs::exists(harness_utils))
	{
		std::cerr << "Missing " << harness_utils << "\n";
		return (finish(2));
	}
	fs::create_directories(build);
	if (!cfg.json && !cfg.web && !cfg.compact)
	{
		std::cout << "Get Next Line Tester\n";
		std::cout << "target: " << cfg.root << "\n";
		std::cout << "mode:   " << (cfg.bonus ? "bonus" : "mandatory") << "\n\n";
	}
	for (int buffer : cfg.buffers)
	{
		RunResult res = run_one(cfg, build, harness, harness_utils, buffer);
		if (run_ok(res))
			passed++;
		total++;
		results.push_back(res);
		if (!cfg.json && !cfg.web && !cfg.compact
			&& (!cfg.summary_only || !run_ok(res)))
			print_result(cfg, res);
		if (cfg.fail_fast && !run_ok(res))
			break ;
	}
	summary.name = cfg.bonus ? "Bonus" : "Mandatory";
	summary.success = (passed == total
		&& total == static_cast<int>(cfg.buffers.size()));
	summary.passed = passed;
	summary.total = total;
	summary.results = results;
	for (const RunResult &res : results)
		summary.duration_ms += res.duration_ms;
	if (cfg.json)
	{
		DiagnosticSummary diag = collect_diagnostics(cfg, tester_dir, false);

		std::cout << "{";
		std::cout << "\"tester\":\"Get Next Line Tester\"";
		std::cout << ",\"version\":\"" << g_version << "\"";
		std::cout << ",\"review\":false";
		std::cout << ",\"target\":";
		print_json_string(std::cout, cfg.root.string());
		std::cout << ",\"command\":";
		print_json_string(std::cout, cfg.command);
		std::cout << ",\"mode\":\"" << (cfg.bonus ? "bonus" : "mandatory")
			<< "\"";
		std::cout << ",\"timeout_ms\":" << cfg.timeout_ms;
		std::cout << ",\"stress\":" << (cfg.stress ? "true" : "false");
		std::cout << ",\"leaks\":" << (cfg.leaks ? "true" : "false");
		std::cout << ",\"norm\":" << (cfg.norm ? "true" : "false");
		std::cout << ",\"malloc_fail\":"
			<< (cfg.malloc_fail ? "true" : "false");
		std::cout << ",\"fd_limit\":" << cfg.fd_limit;
		std::cout << ",\"summary_only\":"
			<< (cfg.summary_only ? "true" : "false");
		std::cout << ",\"fail_fast\":" << (cfg.fail_fast ? "true" : "false");
		std::cout << ",\"success\":" << (summary.success ? "true" : "false");
		std::cout << ",\"passed\":" << passed;
		std::cout << ",\"total\":" << total;
		std::cout << ",\"planned_total\":" << cfg.buffers.size();
		std::cout << ",\"duration_ms\":" << summary.duration_ms;
		std::cout << ",\"buffers\":";
		print_json_buffer_array(std::cout, cfg.buffers);
		std::cout << ",\"case_filter\":";
		print_case_filter_json(std::cout, cfg);
		std::cout << ",\"diagnostics\":";
		print_diagnostics_json(std::cout, diag);
		std::cout << ",\"results\":";
		print_results_json(std::cout, results, true);
		std::cout << "}\n";
	}
	else if (cfg.web)
	{
		print_html_report(std::cout, cfg, tester_dir, {summary}, false,
			summary.success);
	}
	else
	{
		if (cfg.compact)
			std::cout << (summary.success ? "PASS " : "FAIL ")
				<< (cfg.bonus ? "bonus " : "mandatory ")
				<< passed << "/" << total << "\n";
		else
		{
			std::cout << "\nSummary: " << passed << "/" << total
				<< " buffer suites passed\n";
			if (cfg.profile)
				print_profile(results);
			if (cfg.fail_fast && total < static_cast<int>(cfg.buffers.size()))
				std::cout << "Stopped early after first failing buffer suite.\n";
			if (cfg.leaks && !command_exists("valgrind"))
				std::cout << "Note: valgrind not found, leak checks were skipped.\n";
		}
	}
	return (finish(passed == total
		&& total == static_cast<int>(cfg.buffers.size()) ? 0 : 1));
}
