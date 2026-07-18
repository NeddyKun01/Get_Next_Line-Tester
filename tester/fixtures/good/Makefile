NAME = gnl_fixture

SRCS = get_next_line.c get_next_line_utils.c
BONUS_SRCS = get_next_line_bonus.c get_next_line_utils_bonus.c

all:
	@$(CC) -Wall -Wextra -Werror -D BUFFER_SIZE=42 -c $(SRCS)

bonus:
	@$(CC) -Wall -Wextra -Werror -D BUFFER_SIZE=42 -c $(BONUS_SRCS)

clean:
	@rm -f *.o

fclean: clean

re: fclean all
