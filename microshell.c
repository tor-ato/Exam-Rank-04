#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* Error handling functions */
void	print_error(char *msg)
{
	/* Write error message character by character to stderr */
	while (*msg)
	{
		write(2, msg, 1);
		msg++;
	}
}

void	exit_with_error(char *msg)
{
	print_error(msg);
	exit(1);
}

/* Built-in command: cd */
int	try_change_dir(char *path)
{
	/* Attempt to change directory and handle errors */
	if (chdir(path) < 0)
	{
		print_error("error: cd: cannot change directory to ");
		print_error(path);
		print_error("\n");
		return (1);
	}
	return (0);
}

int	handle_cd(char **argv, int arg_count)
{
	/* Check if cd command has correct number of arguments */
	if (arg_count != 2)
	{
		print_error("error: cd: bad arguments\n");
		return (1);
	}
	return (try_change_dir(argv[1]));
}

/* Pipe handling */
void	setup_pipe(int pipe_fd[2], int end)
{
	/* end == 1: sets stdout to write end of pipe */
	/* end == 0: sets stdin to read end of pipe */
	if (dup2(pipe_fd[end], end) == -1)
		exit_with_error("error: fatal\n");
	if (close(pipe_fd[0]) == -1 || close(pipe_fd[1]) == -1)
		exit_with_error("error: fatal\n");
}

/* Command execution */
void	execute_command(char **argv, char **envp)
{
	/* Try to execute the command */
	execve(*argv, argv, envp);
	/* If execve returns, it means the command failed */
	print_error("error: cannot execute ");
	print_error(*argv);
	print_error("\n");
	exit(1);
}

int	create_pipe(int pipe_fd[2])
{
	/* Create a pipe and handle potential errors */
	if (pipe(pipe_fd) == -1)
	{
		exit_with_error("error: fatal\n");
		return (0);
	}
	return (1);
}

int	create_process(void)
{
	int	pid;

	/* Create a child process using fork */
	pid = fork();
	if (pid == -1)
		exit_with_error("error: fatal\n");
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
	/* Handle built-in cd command */
	if (!has_pipe && !strcmp(*argv, "cd"))
		return (handle_cd(argv, arg_count));
	if (has_pipe && !create_pipe(pipe_fd))
		return (1);
	pid = create_process();
	if (pid == 0)
	{
		/* In child process */
		argv[arg_count] = 0;  /* Null terminate array at pipe operator */
		if (has_pipe)
			setup_pipe(pipe_fd, 1);  /* Setup write end of pipe */
		/* Handle cd command in child process */
		if (!strcmp(*argv, "cd"))
			exit(handle_cd(argv, arg_count));
		execute_command(argv, envp);
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
