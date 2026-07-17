#include "harness_utils.h"
#include "get_next_line.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
	static const char	*exact[] = {"1234567890\n", "abc", NULL};
	static const char	*long_expected[] = {
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n",
		"tail",
		NULL
	};

	if (argc != 2)
		return (2);
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
#ifdef GNL_STRESS
	check_stress_files(argv[1]);
#endif
	if (g_failures == 0)
		printf("OK mandatory cases\n");
	return (g_failures ? 1 : 0);
}
