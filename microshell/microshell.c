#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

# define	STDIN	0
# define	STDOUT	1
# define	STDERR	2

# define	TYPE_END	3
# define	TYPE_PIPE	4
# define	TYPE_BREAK	5

typedef struct	s_data
{
	char	**argv;
	int	size;
	int	type;
	int	fd[2];
	struct s_data	*prev;
	struct s_data	*next;
}	t_data;

/* utils */

int	ft_strlen(char *str)
{
	int	i;

	i = 0;
	if (!str)
		return (0);
	while (str[i])
		i++;
	return (i);
}

char	*ft_strdup(char *str)
{
	int	len;
	char	*new;

	len = ft_strlen(str);
	if (!str)
		return (NULL);
	if (!(new = (char *)malloc(sizeof(char) * (len + 1))))
		return (NULL);
	new[len] = '\0';
	while (len-- > 0)
		new[len] = str[len];
	return (new);
}

/* error */

void	err_fatal(void)
{
	write(STDERR, "error: fatal\n", ft_strlen("error: fatal\n"));
	exit(1);
}

int	err_cd_arg(void)
{
	write(STDERR, "error: cd: bad arguments\n",
			ft_strlen("error: cd: bad arguments\n"));
	return (1);
}

int	err_cd_path(char *str)
{
	write(STDERR, "error: cd: cannot change directory to ",
			ft_strlen("error: cd: cannot change directory to \n"));
	write(STDERR, str, ft_strlen(str));
	write(STDERR, "\n", 1);
	return (1);
}

int	err_exec(char *str)
{
	write(STDERR, "error: cannot execute ",
			ft_strlen("error: cannot execute "));
	write(STDERR, str, ft_strlen(str));
	write(STDERR, "\n", 1);
	return (1);
}

/* parsing */

void	ft_lstadd_back(t_data **ptr, t_data *new)
{
	t_data	*temp;

	if (!(*ptr))
		*ptr = new;
	else
	{
		temp = *ptr;
		while (temp->next)
			temp = temp->next;
		temp->next = new;
		new->prev = temp;
	}
}

int	size_of_argv(char **argv)
{
	int	i;

	i = 0;
	while (argv[i] && strcmp(argv[i], "|") != 0 && strcmp(argv[i], ";") != 0)
		i++;
	return (i);
}

int	check_type_end(char *argv)
{
	if (!argv)
		return (TYPE_END);
	if (strcmp(argv, "|") == 0)
		return (TYPE_PIPE);
	if (strcmp(argv, ";") == 0)
		return (TYPE_BREAK);
	return (0);
}

int	parse_argv(t_data **ptr, char **argv)
{
	int	size;
	t_data	*new;

	size = size_of_argv(argv);
	if (!(new = (t_data *)malloc(sizeof(t_data))))
		err_fatal();
	if (!(new->argv = (char **)malloc(sizeof(char *) * (size + 1))))
		err_fatal();
	new->size = size;
	new->prev = NULL;
	new->next = NULL;
	new->argv[size] = NULL;
	while (size-- > 0)
		new->argv[size] = ft_strdup(argv[size]);
	new->type = check_type_end(argv[new->size]);
	ft_lstadd_back(ptr, new);
	return (new->size);
}

/* exec */

void	ft_exec(t_data *temp, char **env)
{
	pid_t	pid;
	int	status;
	int	pipe_flag;

	pipe_flag = 0;
	if (temp->type == TYPE_PIPE
			|| (temp->prev && temp->prev->type == TYPE_PIPE))
	{
		pipe_flag = 1;
		if (pipe(temp->fd))
			err_fatal();
	}
	pid = fork();
	if (pid < 0)
		err_fatal();
	else if (pid == 0)	//child
	{
		if (temp->type == TYPE_PIPE
				&& (dup2(temp->fd[STDOUT], STDOUT) < 0))
			err_fatal();
		if (temp->prev && temp->prev->type == TYPE_PIPE
				&& (dup2(temp->prev->fd[STDIN], STDIN) < 0))
			err_fatal();
		if ((execve(temp->argv[0], temp->argv, env)) < 0)
			err_exec(temp->argv[0]);
		exit(0);
	}
	else		//parent
	{
		waitpid(pid, &status, 0);
		if (pipe_flag == 1)
		{
			close(temp->fd[STDOUT]);
			if (!temp->next || temp->type == TYPE_BREAK)
				close(temp->fd[STDIN]);
		}
		if (temp->prev && temp->prev->type == TYPE_PIPE)
			close(temp->prev->fd[STDIN]);
	}
}

void	exec_cmds(t_data *ptr, char **env)
{
	t_data	*temp;

	temp = ptr;
	while (temp)
	{
		if (strcmp("cd", temp->argv[0]) == 0)
		{
			if (temp->size != 2)
				err_cd_arg();
			else if (chdir(temp->argv[1]))
				err_cd_path(temp->argv[1]);
		}
		else
			ft_exec(temp, env);
		temp = temp->next;
	}
}

/* main */

void	ft_free(t_data *ptr)
{
	t_data	*temp;
	int	i;

	while (ptr)
	{
		temp = ptr->next;
		i = 0;
		while (i < ptr->size)
			free(ptr->argv[i++]);
		free(ptr->argv);
		free(ptr);
		ptr = temp;
	}
	ptr = NULL;
}

int	main(int argc, char **argv, char **env)
{
	t_data	*ptr = NULL;
	int	i;

	i = 1;
	if (argc > 1)
	{
		while (argv[i])
		{
			if (strcmp(argv[i], ";") == 0)
			{
				i++;
				continue ;
			}
			i += parse_argv(&ptr, &argv[i]);
			if (!argv[i])
				break ;
			else
				i++;
		}
		if (ptr)
			exec_cmds(ptr, env);
		ft_free(ptr);
	}
	return (0);
}
