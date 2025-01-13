#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void	perror(char *msg)
{
	while (*msg)
		write(2, msg++, 1);
}

void	perror_exit(char *msg)
{
	perror(msg);
	exit(1);
}

int	xchdir(char *path)
{
	if (chdir(path) > -1)
		return (0);
	perror("error: cd: cannot change directory to ");
	perror(path);
	perror("\n");
	return (1);
}

int	handle_cd(char **argv, int arg_count)
{
	if (arg_count == 2)
		return (xchdir(argv[1]));
	perror("error: cd: bad arguments\n");
	return (1);
}

/* Pipe handling */
void	setup_pipe(int pipe_fd[2], int end)
{
	/* end == 1: sets stdout to write end of pipe */
	/* end == 0: sets stdin to read end of pipe */
	if (dup2(pipe_fd[end], end) == -1)
		perror_exit("error: fatal\n");
	if (close(pipe_fd[0]) == -1 || close(pipe_fd[1]) == -1)
		perror_exit("error: fatal\n");
}

void	xexecve(char **argv, char **envp)
{
	execve(*argv, argv, envp);
	perror("error: cannot execute ");
	perror(*argv);
	perror("\n");
	exit(1);
}

int	xpipe(int pipe_fd[2])
{
	if (pipe(pipe_fd) == 0)
		return (1);
	perror_exit("error: fatal\n");
	return (0);
}

int	xfork(void)
{
	int	pid;

	pid = fork();
	if (pid == -1)
		perror_exit("error: fatal\n");
	return (pid);
}

int	exec(char **argv, int arg_count, char **envp)
{
	int	has_pipe;
	int	pipe_fd[2];
	int	pid;
	int	status;

	/* Check if command includes a pipe operator */
	has_pipe = argv[arg_count] && !strcmp(argv[arg_count], "|");
	if (!has_pipe && !strcmp(*argv, "cd"))
		return (handle_cd(argv, arg_count));
	if (has_pipe && !xpipe(pipe_fd))
		return (1);
	pid = xfork();
	if (pid == 0)
	{
		/* In child process */
		argv[arg_count] = 0;  /* Null terminate array at pipe operator */
		if (has_pipe)
			setup_pipe(pipe_fd, 1);  /* Setup write end of pipe */
		/* Handle cd command in child process */
		if (!strcmp(*argv, "cd"))
			exit(handle_cd(argv, arg_count));
		xexecve(argv, envp);
	}
	/* In parent process */
	waitpid(pid, &status, 0);  /* Wait for child to finish */
	if (has_pipe)
		setup_pipe(pipe_fd, 0);  /* Setup read end of pipe */
	return (WIFEXITED(status) && WEXITSTATUS(status));
}

int	main(int argc, char **argv, char **envp)
{
	int	i;
	int	status;

	(void)argc;
	i = 0;
	status = 0;
	while (argv[i])
	{
		/* Move pointer to next argument after last delimiter */
		argv += i + 1;
		i = 0;
		/* Count arguments until next delimiter (| or ;) */
		while (argv[i] && strcmp(argv[i], "|") && strcmp(argv[i], ";"))
			i++;
		/* Execute command if arguments exist */
		if (i)
			status = exec(argv, i, envp);
	}
	return (status);
}
