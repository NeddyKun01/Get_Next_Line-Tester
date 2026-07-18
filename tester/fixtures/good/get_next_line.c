#include "get_next_line.h"
#include <stdlib.h>
#include <unistd.h>

static char	*append_char(char *line, size_t *len, char c)
{
	char	*next;
	size_t	i;

	next = malloc(*len + 2);
	if (!next)
	{
		free(line);
		return (NULL);
	}
	i = 0;
	while (i < *len)
	{
		next[i] = line[i];
		i++;
	}
	next[*len] = c;
	(*len)++;
	next[*len] = '\0';
	free(line);
	return (next);
}

char	*get_next_line(int fd)
{
	char	*line;
	char	c;
	size_t	len;
	ssize_t	n;

	if (fd < 0)
		return (NULL);
	line = NULL;
	len = 0;
	while ((n = read(fd, &c, 1)) > 0)
	{
		line = append_char(line, &len, c);
		if (!line || c == '\n')
			break ;
	}
	if (n < 0 || (n == 0 && len == 0))
	{
		free(line);
		return (NULL);
	}
	return (line);
}
