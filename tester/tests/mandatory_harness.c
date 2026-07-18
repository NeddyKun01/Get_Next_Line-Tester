#include "harness_utils.h"
#include "get_next_line.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char	*g_only = NULL;
static const char	*g_skip = NULL;

static int	should_run(const char *name)
{
	if (g_only && strcmp(g_only, name) != 0)
		return (0);
	if (g_skip && strcmp(g_skip, name) == 0)
		return (0);
	return (1);
}

static void	check_file(const char *dir, const char *name,
	const char *content, const char **expected)
{
	char	*path;
	char	label[128];
	int		fd;
	int		i;

	path = join_path(dir, name);
	i = 0;
	write_text(path, content);
	fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		perror(path);
		exit(2);
	}
	while (expected[i])
	{
		snprintf(label, sizeof(label), "%s[%d]", name, i);
		check_line(label, get_next_line(fd), expected[i]);
		i++;
	}
	check_line(name, get_next_line(fd), NULL);
	check_line(name, get_next_line(fd), NULL);
	close(fd);
	free(path);
}

static void	check_pipe(void)
{
	int	fds[2];

	if (pipe(fds) < 0)
	{
		perror("pipe");
		exit(2);
	}
	write(fds[1], "pipe-a\npipe-b\nlast", 18);
	close(fds[1]);
	check_line("pipe[0]", get_next_line(fds[0]), "pipe-a\n");
	check_line("pipe[1]", get_next_line(fds[0]), "pipe-b\n");
	check_line("pipe[2]", get_next_line(fds[0]), "last");
	check_line("pipe eof", get_next_line(fds[0]), NULL);
	check_line("pipe eof repeat", get_next_line(fds[0]), NULL);
	close(fds[0]);
}

static void	check_stdin_fd(const char *dir)
{
	char	*path;
	int		fd;

	path = join_path(dir, "stdin.txt");
	write_text(path, "stdin-a\nstdin-b\nstdin-last");
	fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		perror(path);
		exit(2);
	}
	if (dup2(fd, STDIN_FILENO) < 0)
	{
		perror("dup2");
		exit(2);
	}
	close(fd);
	check_line("stdin[0]", get_next_line(STDIN_FILENO), "stdin-a\n");
	check_line("stdin[1]", get_next_line(STDIN_FILENO), "stdin-b\n");
	check_line("stdin[2]", get_next_line(STDIN_FILENO), "stdin-last");
	check_line("stdin eof", get_next_line(STDIN_FILENO), NULL);
	free(path);
}

static void	check_closed_fd(const char *dir)
{
	char	*path;
	int		fd;

	path = join_path(dir, "closed.txt");
	write_text(path, "closed\n");
	fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		perror(path);
		exit(2);
	}
	close(fd);
	check_line("closed fd", get_next_line(fd), NULL);
	free(path);
}

static void	check_boundary_case(const char *dir, const char *name, int len)
{
	char	*line;
	char	*line_nl;
	char	*content;
	const char	*expected[] = {NULL, "tail", NULL};

	if (len < 1)
		len = 1;
	line = repeat_char('b', (size_t)len);
	line_nl = join2(line, "\n");
	content = join3(line, "\n", "tail");
	expected[0] = line_nl;
	check_file(dir, name, content, expected);
	free(line);
	free(line_nl);
	free(content);
}

static void	check_buffer_edges(const char *dir)
{
	check_boundary_case(dir, "buffer_size_minus_one.txt", BUFFER_SIZE - 1);
	check_boundary_case(dir, "buffer_size_exact.txt", BUFFER_SIZE);
	check_boundary_case(dir, "buffer_size_plus_one.txt", BUFFER_SIZE + 1);
}

static void	check_double_buffer_edges(const char *dir)
{
	check_boundary_case(dir, "buffer_size_double.txt", BUFFER_SIZE * 2);
	check_boundary_case(dir, "buffer_size_double_plus_one.txt",
		BUFFER_SIZE * 2 + 1);
}

static void	check_read_error_paths(void)
{
	check_line("read-zero invalid fd", get_next_line(-42), NULL);
	check_line("read-zero invalid repeat", get_next_line(-42), NULL);
}

#ifdef GNL_MALLOC_FAIL
static void	check_malloc_failure(const char *dir)
{
	char	*path;
	int		fd;

	path = join_path(dir, "malloc_fail.txt");
	write_text(path, "malloc-fail\n");
	fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		perror(path);
		exit(2);
	}
	harness_malloc_fail_after(0);
	check_line("malloc fail first allocation", get_next_line(fd), NULL);
	harness_malloc_fail_disable();
	close(fd);
	free(path);
}
#endif

#ifdef GNL_STRESS
static void	check_stress_files(const char *dir)
{
	char	*line_10k;
	char	*line_10k_nl;
	char	*content_10k;
	char	*line_100k;
	char	*content_100k_tail;
	char	*line_100k_nl;
	const char	*expected_10k[] = {NULL, "tail", NULL};
	const char	*expected_100k[] = {NULL, NULL};
	const char	*expected_100k_tail[] = {NULL, "z", NULL};

	line_10k = repeat_char('x', 10000);
	line_10k_nl = join2(line_10k, "\n");
	content_10k = join3(line_10k, "\n", "tail");
	expected_10k[0] = line_10k_nl;
	check_file(dir, "stress_10k_newline_tail.txt", content_10k, expected_10k);
	free(line_10k);
	free(line_10k_nl);
	free(content_10k);
	line_100k = repeat_char('y', 100000);
	expected_100k[0] = line_100k;
	check_file(dir, "stress_100k_no_newline.txt", line_100k, expected_100k);
	content_100k_tail = join3(line_100k, "\n", "z");
	line_100k_nl = join2(line_100k, "\n");
	expected_100k_tail[0] = line_100k_nl;
	check_file(dir, "stress_100k_newline_tail.txt", content_100k_tail,
		expected_100k_tail);
	free(line_100k);
	free(content_100k_tail);
	free(line_100k_nl);
}
#endif

int	main(int argc, char **argv)
{
	static const char	*empty[] = {NULL};
	static const char	*one_no_nl[] = {"hello", NULL};
	static const char	*one_nl[] = {"hello\n", NULL};
	static const char	*many[] = {"alpha\n", "beta\n", "gamma", NULL};
	static const char	*blank[] = {"\n", "\n", "abc\n", "\n", "last", NULL};
	static const char	*newline_only[] = {"\n", NULL};
	static const char	*many_newlines[] = {"\n", "\n", "\n", "\n", NULL};
	static const char	*exact[] = {"1234567890\n", "abc", NULL};
	static const char	*long_expected[] = {
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n",
		"tail",
		NULL
	};

	if (argc < 2)
		return (2);
	if (argc > 2 && argv[2][0])
		g_only = argv[2];
	if (argc > 3 && argv[3][0])
		g_skip = argv[3];
	if (should_run("invalid-fd"))
		check_line("invalid fd", get_next_line(-1), NULL);
	if (should_run("closed-fd"))
		check_closed_fd(argv[1]);
	if (should_run("empty"))
		check_file(argv[1], "empty.txt", "", empty);
	if (should_run("one-line"))
	{
		check_file(argv[1], "one_no_nl.txt", "hello", one_no_nl);
		check_file(argv[1], "one_nl.txt", "hello\n", one_nl);
	}
	if (should_run("many"))
		check_file(argv[1], "many.txt", "alpha\nbeta\ngamma", many);
	if (should_run("blank"))
		check_file(argv[1], "blank.txt", "\n\nabc\n\nlast", blank);
	if (should_run("newline-only"))
		check_file(argv[1], "newline_only.txt", "\n", newline_only);
	if (should_run("many-newlines"))
		check_file(argv[1], "many_newlines.txt", "\n\n\n\n", many_newlines);
	if (should_run("buffer-edge"))
		check_file(argv[1], "exact.txt", "1234567890\nabc", exact);
	if (should_run("long"))
		check_file(argv[1], "long.txt",
			"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
			"tail",
			long_expected);
	if (should_run("pipe"))
		check_pipe();
	if (should_run("stdin"))
		check_stdin_fd(argv[1]);
	if (should_run("buffer-edge"))
		check_buffer_edges(argv[1]);
	if (should_run("double-buffer"))
		check_double_buffer_edges(argv[1]);
	if (should_run("read-zero"))
		check_read_error_paths();
#ifdef GNL_MALLOC_FAIL
	if (should_run("malloc-fail"))
		check_malloc_failure(argv[1]);
#endif
#ifdef GNL_STRESS
	if (should_run("stress"))
		check_stress_files(argv[1]);
#endif
	if (g_failures == 0)
		printf("OK mandatory cases\n");
	return (g_failures ? 1 : 0);
}
