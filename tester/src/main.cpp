#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

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

static std::string read_file(const fs::path &path)
{
	std::ifstream in(path);
	std::ostringstream ss;

	ss << in.rdbuf();
	return (ss.str());
}

static bool write_file(const fs::path &path, const std::string &content)
{
	std::ofstream out(path);

	if (!out)
		return (false);
	out << content;
	return (true);
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

static const char *harness_source(bool bonus)
{
	if (bonus)
		return (R"EOF(
#include "get_next_line_bonus.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures = 0;

static void write_text(const char *path, const char *text)
{
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
	{
		perror(path);
		exit(2);
	}
	write(fd, text, strlen(text));
	close(fd);
}

static char *join_path(const char *dir, const char *name)
{
	size_t len = strlen(dir) + strlen(name) + 2;
	char *path = malloc(len);
	if (!path)
		exit(2);
	snprintf(path, len, "%s/%s", dir, name);
	return path;
}

static void check_line(const char *label, char *got, const char *expected)
{
	if ((got == NULL && expected != NULL)
		|| (got != NULL && expected == NULL)
		|| (got != NULL && strcmp(got, expected) != 0))
	{
		printf("NOK %s expected=%s got=%s\n", label,
			expected ? expected : "(null)", got ? got : "(null)");
		failures++;
	}
	free(got);
}

int main(int argc, char **argv)
{
	char *a_path;
	char *b_path;
	int a;
	int b;

	if (argc != 2)
		return 2;
	check_line("invalid fd", get_next_line(-1), NULL);
	a_path = join_path(argv[1], "bonus_a.txt");
	b_path = join_path(argv[1], "bonus_b.txt");
	write_text(a_path, "a1\na2\na3");
	write_text(b_path, "b1\n\nb3\n");
	a = open(a_path, O_RDONLY);
	b = open(b_path, O_RDONLY);
	check_line("a first", get_next_line(a), "a1\n");
	check_line("b first", get_next_line(b), "b1\n");
	check_line("a second", get_next_line(a), "a2\n");
	check_line("b blank", get_next_line(b), "\n");
	check_line("b third", get_next_line(b), "b3\n");
	check_line("a third eof line", get_next_line(a), "a3");
	check_line("a eof", get_next_line(a), NULL);
	check_line("b eof", get_next_line(b), NULL);
	close(a);
	close(b);
	free(a_path);
	free(b_path);
	if (failures == 0)
		printf("OK bonus interleaved fds\n");
	return failures ? 1 : 0;
}
)EOF");
	return (R"EOF(
#include "get_next_line.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures = 0;

static void write_text(const char *path, const char *text)
{
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
	{
		perror(path);
		exit(2);
	}
	write(fd, text, strlen(text));
	close(fd);
}

static char *join_path(const char *dir, const char *name)
{
	size_t len = strlen(dir) + strlen(name) + 2;
	char *path = malloc(len);
	if (!path)
		exit(2);
	snprintf(path, len, "%s/%s", dir, name);
	return path;
}

static void check_line(const char *label, char *got, const char *expected)
{
	if ((got == NULL && expected != NULL)
		|| (got != NULL && expected == NULL)
		|| (got != NULL && strcmp(got, expected) != 0))
	{
		printf("NOK %s expected=%s got=%s\n", label,
			expected ? expected : "(null)", got ? got : "(null)");
		failures++;
	}
	free(got);
}

static void check_file(const char *dir, const char *name,
	const char *content, const char **expected)
{
	char *path = join_path(dir, name);
	int fd;
	int i = 0;

	write_text(path, content);
	fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		perror(path);
		exit(2);
	}
	while (expected[i])
	{
		char label[128];
		snprintf(label, sizeof(label), "%s[%d]", name, i);
		check_line(label, get_next_line(fd), expected[i]);
		i++;
	}
	check_line(name, get_next_line(fd), NULL);
	check_line(name, get_next_line(fd), NULL);
	close(fd);
	free(path);
}

int main(int argc, char **argv)
{
	static const char *empty[] = {NULL};
	static const char *one_no_nl[] = {"hello", NULL};
	static const char *one_nl[] = {"hello\n", NULL};
	static const char *many[] = {"alpha\n", "beta\n", "gamma", NULL};
	static const char *blank[] = {"\n", "\n", "abc\n", "\n", "last", NULL};
	static const char *exact[] = {"1234567890\n", "abc", NULL};
	static const char *long_expected[] = {
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n",
		"tail",
		NULL
	};

	if (argc != 2)
		return 2;
	check_line("invalid fd", get_next_line(-1), NULL);
	check_file(argv[1], "empty.txt", "", empty);
	check_file(argv[1], "one_no_nl.txt", "hello", one_no_nl);
	check_file(argv[1], "one_nl.txt", "hello\n", one_nl);
	check_file(argv[1], "many.txt", "alpha\nbeta\ngamma", many);
	check_file(argv[1], "blank.txt", "\n\nabc\n\nlast", blank);
	check_file(argv[1], "exact.txt", "1234567890\nabc", exact);
	check_file(argv[1], "long.txt",
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
		"tail",
		long_expected);
	if (failures == 0)
		printf("OK mandatory cases\n");
	return failures ? 1 : 0;
}
)EOF");
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

static std::string compile_command(const Config &cfg, const fs::path &harness,
	const fs::path &exe, int buffer)
{
	std::ostringstream cmd;

	cmd << "cc -Wall -Wextra -Werror -D BUFFER_SIZE=" << buffer
		<< " -I " << quote(cfg.root.string()) << " "
		<< quote(harness.string()) << " ";
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

static RunResult run_one(const Config &cfg, const fs::path &build, int buffer)
{
	RunResult res;
	fs::path run_dir = build / ("buffer_" + std::to_string(buffer)
		+ (cfg.bonus ? "_bonus" : ""));
	fs::path harness = run_dir / "harness.c";
	fs::path exe = run_dir / "gnl_case";
	fs::path compile_log = run_dir / "compile.log";
	fs::path test_log = run_dir / "test.log";
	fs::path leak_log = run_dir / "leaks.log";
	int code;

	res.buffer = buffer;
	fs::create_directories(run_dir);
	write_file(harness, harness_source(cfg.bonus));
	code = run_command(compile_command(cfg, harness, exe, buffer), compile_log);
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
	fs::path build = fs::current_path() / "tester" / "build" / "runs";
	int passed = 0;

	if (cfg.help)
	{
		print_help();
		return (0);
	}
	cfg.root = fs::weakly_canonical(cfg.root);
	if (!validate_root(cfg))
		return (2);
	fs::create_directories(build);
	std::cout << "Get Next Line Tester\n";
	std::cout << "target: " << cfg.root << "\n";
	std::cout << "mode:   " << (cfg.bonus ? "bonus" : "mandatory") << "\n\n";
	for (int buffer : cfg.buffers)
	{
		RunResult res = run_one(cfg, build, buffer);
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
