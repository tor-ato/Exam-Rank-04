#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>

void	perror(char *str)
{
	while (*str)
		write(2, str++, 1);
}

void	perror_exit(char *str)
{
	perror(str);
	exit(1);
}

int	xchdir(char *path)
{
	if (chdir(path) == 0)
		return (0);
	perror("error: cd: cannot change directory to ");
	perror(path);
	perror("\n");
	return (1);
}

int	handle_cd(char *pathname, int arg_count)
{
	if (arg_count == 2)
		return (xchdir(pathname));
	perror("error: cd: bad arguments\n");
	return (1);
}

void	xexecve(char *pathname, char **argv, char **envp)
{
	execve(pathname, argv, envp);
	perror("error: cannot execute ");
	perror(pathname);
	perror_exit("\n");
}

int	xfork(void)
{
	int	pid;

	pid = fork();
	if (pid == -1)
		perror_exit("error: fatal\n");
	return (pid);
}

void	setup_pipe(int pipe_fd[2], int end)
{
	if (dup2(pipe_fd[end], end) == -1)
		perror_exit("error: fatal\n");
	if (close(pipe_fd[0]) == -1 || close(pipe_fd[1]) == -1)
		perror_exit("error: fatal\n");
}

void	xpipe(int pipe_fd[2])
{
	if (pipe(pipe_fd) == -1)
		perror_exit("error: fatal\n");
}

int	exec(char **argv, int arg_count, char **envp)
{
	int	has_pipe;
	int	pipe_fd[2];
	int	pid;
	int	status;

	has_pipe = false;
	if (argv[arg_count] && !strcmp(argv[arg_count], "|"))
		has_pipe = true;
	if (!strcmp(*argv, "cd"))
		return (handle_cd(argv[1], arg_count));
	if (has_pipe)
		xpipe(pipe_fd);
	pid = xfork();
	if (pid == 0)
	{
		if (has_pipe)
			setup_pipe(pipe_fd, 1);
		argv[arg_count] = 0;
		xexecve(*argv, argv, envp);
	}
	waitpid(pid, &status, 0);
	if (has_pipe)
		setup_pipe(pipe_fd, 0);
	return (WIFEXITED(status) && WEXITSTATUS(status));
}

int	main(int argc, char **argv, char **envp)
{
	int	arg_count;
	int	status;

	(void)argc;
	arg_count = 0;
	status = 0;
	while (argv[arg_count])
	{
		argv += arg_count + 1;
		arg_count = 0;
		while (argv[arg_count] && strcmp(argv[arg_count], "|") && strcmp(argv[arg_count], ";"))
			arg_count++;
		if (arg_count)
			status = exec(argv, arg_count, envp);
	}
	return (status);
}
