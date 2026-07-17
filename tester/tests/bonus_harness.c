#include "harness_utils.h"
#include "get_next_line_bonus.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void	check_many_fds(const char *dir)
{
	char	*path;
	char	*name;
	char	*content;
	char	*label;
	char	*expected;
	int		fds[8];
	int		i;

	i = 0;
	while (i < 8)
	{
		name = format_text("bonus_many_%d.txt", i);
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
	while (i < 8)
	{
		label = format_text("many fd first %d", i);
		expected = format_text("fd%d-a\n", i);
		check_line(label, get_next_line(fds[i]), expected);
		free(label);
		free(expected);
		i++;
	}
	i = 0;
	while (i < 8)
	{
		label = format_text("many fd second %d", i);
		expected = format_text("fd%d-b\n", i);
		check_line(label, get_next_line(fds[i]), expected);
		free(label);
		free(expected);
		i++;
	}
	i = 0;
	while (i < 8)
	{
		label = format_text("many fd last %d", i);
		expected = format_text("fd%d-last", i);
		check_line(label, get_next_line(fds[i]), expected);
		free(label);
		free(expected);
		i++;
	}
	i = 0;
	while (i < 8)
	{
		label = format_text("many fd eof %d", i);
		check_line(label, get_next_line(fds[i]), NULL);
		free(label);
		close(fds[i]);
		i++;
	}
}

int	main(int argc, char **argv)
{
	char	*a_path;
	char	*b_path;
	int		a;
	int		b;

	if (argc != 2)
		return (2);
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
	check_many_fds(argv[1]);
	if (g_failures == 0)
		printf("OK bonus interleaved fds\n");
	return (g_failures ? 1 : 0);
}
