#include "get_next_line_bonus.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int	failures = 0;

static void	write_text(const char *path, const char *text)
{
	int	fd;

	fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
	{
		perror(path);
		exit(2);
	}
	write(fd, text, strlen(text));
	close(fd);
}

static char	*join_path(const char *dir, const char *name)
{
	size_t	len;
	char	*path;

	len = strlen(dir) + strlen(name) + 2;
	path = malloc(len);
	if (!path)
		exit(2);
	snprintf(path, len, "%s/%s", dir, name);
	return (path);
}

static void	check_line(const char *label, char *got, const char *expected)
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
	if (failures == 0)
		printf("OK bonus interleaved fds\n");
	return (failures ? 1 : 0);
}
