#ifndef HARNESS_UTILS_H
# define HARNESS_UTILS_H

# include <stddef.h>

extern int	g_failures;

void	write_text(const char *path, const char *text);
char	*join_path(const char *dir, const char *name);
char	*repeat_char(char value, size_t count);
char	*format_text(const char *format, int value);
char	*format_text3(const char *format, int a, int b, int c);
char	*join2(const char *a, const char *b);
char	*join3(const char *a, const char *b, const char *c);
void	check_line(const char *label, char *got, const char *expected);

#endif
