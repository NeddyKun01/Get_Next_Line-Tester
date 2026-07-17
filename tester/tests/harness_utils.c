#include "harness_utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int	g_failures = 0;

void	write_text(const char *path, const char *text)
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

char	*join_path(const char *dir, const char *name)
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

void	check_line(const char *label, char *got, const char *expected)
{
	if ((got == NULL && expected != NULL)
		|| (got != NULL && expected == NULL)
		|| (got != NULL && strcmp(got, expected) != 0))
	{
		printf("NOK %s expected=%s got=%s\n", label,
			expected ? expected : "(null)", got ? got : "(null)");
		g_failures++;
	}
	free(got);
}
