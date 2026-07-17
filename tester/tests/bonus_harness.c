#include "harness_utils.h"
#include "get_next_line_bonus.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
	if (g_failures == 0)
		printf("OK bonus interleaved fds\n");
	return (g_failures ? 1 : 0);
}
