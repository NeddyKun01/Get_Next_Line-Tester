#ifndef HARNESS_UTILS_H
# define HARNESS_UTILS_H

extern int	g_failures;

void	write_text(const char *path, const char *text);
char	*join_path(const char *dir, const char *name);
void	check_line(const char *label, char *got, const char *expected);

#endif
