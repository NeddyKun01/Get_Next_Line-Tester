#include "harness_utils.h"
#include "get_next_line_bonus.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char	*g_only = NULL;
static const char	*g_skip = NULL;
static int			g_fd_limit = 32;

static int	should_run(const char *name)
{
	if (g_only && strcmp(g_only, name) != 0)
		return (0);
	if (g_skip && strcmp(g_skip, name) == 0)
		return (0);
	return (1);
}

static void	check_many_fds_count(const char *dir, int count, const char *prefix)
{
	char	*path;
	char	*name;
	char	*content;
	char	*label;
	char	*expected;
	int		*fds;
	int		i;

	fds = malloc(sizeof(int) * (size_t)count);
	if (!fds)
		exit(2);
	i = 0;
	while (i < count)
	{
		name = format_text(prefix, i);
		path = join_path(dir, name);
		content = format_text3("fd%d-a\nfd%d-b\nfd%d-last", i, i, i);
		write_text(path, content);
		fds[i] = open(path, O_RDONLY);
		if (fds[i] < 0)
		{
			perror(path);
			exit(2);
		}
		free(name);
		free(path);
		free(content);
		i++;
	}
	i = 0;
	while (i < count)
	{
		label = format_text("many fd first %d", i);
		expected = format_text("fd%d-a\n", i);
		check_line(label, get_next_line(fds[i]), expected);
		free(label);
		free(expected);
		i++;
	}
	i = 0;
	while (i < count)
	{
		label = format_text("many fd second %d", i);
		expected = format_text("fd%d-b\n", i);
		check_line(label, get_next_line(fds[i]), expected);
		free(label);
		free(expected);
		i++;
	}
	i = 0;
	while (i < count)
	{
		label = format_text("many fd last %d", i);
		expected = format_text("fd%d-last", i);
		check_line(label, get_next_line(fds[i]), expected);
		free(label);
		free(expected);
		i++;
	}
	i = 0;
	while (i < count)
	{
		label = format_text("many fd eof %d", i);
		check_line(label, get_next_line(fds[i]), NULL);
		free(label);
		close(fds[i]);
		i++;
	}
	free(fds);
}

static void	check_many_fds(const char *dir)
{
	check_many_fds_count(dir, 8, "bonus_many_%d.txt");
}

static void	check_wide_fds(const char *dir)
{
	check_many_fds_count(dir, g_fd_limit, "bonus_wide_%d.txt");
}

int	main(int argc, char **argv)
{
	char	*a_path;
	char	*b_path;
	int		a;
	int		b;

	if (argc < 2)
		return (2);
	if (argc > 2 && argv[2][0])
		g_only = argv[2];
	if (argc > 3 && argv[3][0])
		g_skip = argv[3];
	if (argc > 4 && atoi(argv[4]) > 0)
		g_fd_limit = atoi(argv[4]);
	if (should_run("invalid-fd"))
		check_line("invalid fd", get_next_line(-1), NULL);
	if (should_run("bonus-basic"))
	{
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
	}
	if (should_run("bonus-many-fds"))
		check_many_fds(argv[1]);
	if (should_run("bonus-wide-fds"))
		check_wide_fds(argv[1]);
	if (g_failures == 0)
		printf("OK bonus interleaved fds\n");
	return (g_failures ? 1 : 0);
}
