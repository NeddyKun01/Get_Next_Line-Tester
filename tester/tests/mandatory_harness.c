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
	if (g_failures == 0)
		printf("OK mandatory cases\n");
	return (g_failures ? 1 : 0);
}
