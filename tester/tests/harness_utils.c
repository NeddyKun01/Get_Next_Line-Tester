#include "harness_utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int	g_failures = 0;

#ifdef GNL_MALLOC_FAIL
static int	g_malloc_fail_enabled = 0;
static int	g_malloc_fail_after = -1;

void	*__real_malloc(size_t size);

void	*__wrap_malloc(size_t size)
{
	if (g_malloc_fail_enabled)
	{
		if (g_malloc_fail_after <= 0)
			return (NULL);
		g_malloc_fail_after--;
	}
	return (__real_malloc(size));
}

void	harness_malloc_fail_after(int count)
{
	g_malloc_fail_enabled = 1;
	g_malloc_fail_after = count;
}

void	harness_malloc_fail_disable(void)
{
	g_malloc_fail_enabled = 0;
	g_malloc_fail_after = -1;
}
#endif

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

char	*repeat_char(char value, size_t count)
{
	char	*text;
	size_t	i;

	text = malloc(count + 1);
	if (!text)
		exit(2);
	i = 0;
	while (i < count)
		text[i++] = value;
	text[count] = '\0';
	return (text);
}

char	*format_text(const char *format, int value)
{
	int		len;
	char	*text;

	len = snprintf(NULL, 0, format, value);
	if (len < 0)
		exit(2);
	text = malloc((size_t)len + 1);
	if (!text)
		exit(2);
	snprintf(text, (size_t)len + 1, format, value);
	return (text);
}

char	*format_text3(const char *format, int a, int b, int c)
{
	int		len;
	char	*text;

	len = snprintf(NULL, 0, format, a, b, c);
	if (len < 0)
		exit(2);
	text = malloc((size_t)len + 1);
	if (!text)
		exit(2);
	snprintf(text, (size_t)len + 1, format, a, b, c);
	return (text);
}

char	*join2(const char *a, const char *b)
{
	size_t	a_len;
	size_t	b_len;
	char	*text;

	a_len = strlen(a);
	b_len = strlen(b);
	text = malloc(a_len + b_len + 1);
	if (!text)
		exit(2);
	memcpy(text, a, a_len);
	memcpy(text + a_len, b, b_len);
	text[a_len + b_len] = '\0';
	return (text);
}

char	*join3(const char *a, const char *b, const char *c)
{
	char	*left;
	char	*text;

	left = join2(a, b);
	text = join2(left, c);
	free(left);
	return (text);
}

static void	print_value(const char *label, const char *value)
{
	size_t	len;
	size_t	i;
	size_t	limit;

	if (!value)
	{
		printf("%s=(null)", label);
		return ;
	}
	len = strlen(value);
	limit = len < 60 ? len : 60;
	printf("%s(len=%zu)=\"", label, len);
	i = 0;
	while (i < limit)
	{
		if (value[i] == '\n')
			printf("\\n");
		else if (value[i] == '\t')
			printf("\\t");
		else
			putchar(value[i]);
		i++;
	}
	if (len > limit)
		printf("...");
	printf("\"");
}

void	check_line(const char *label, char *got, const char *expected)
{
	if ((got == NULL && expected != NULL)
		|| (got != NULL && expected == NULL)
		|| (got != NULL && strcmp(got, expected) != 0))
	{
		printf("NOK %s ", label);
		print_value("expected", expected);
		printf(" ");
		print_value("got", got);
		printf("\n");
		g_failures++;
	}
	free(got);
}
