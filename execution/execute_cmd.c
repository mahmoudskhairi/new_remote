/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_cmd.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mskhairi <mskhairi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/22 17:05:09 by rmarzouk          #+#    #+#             */
/*   Updated: 2024/07/29 23:02:39 by mskhairi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution.h"

extern	int	exit_status;

int	handle_pipes(t_simple_cmd *cmd)
{
	//if there is pipe or not : [0 = no pipe]  [1 = before] [2 = after] [3 = befor & after]
	if (cmd->pipe_flag == AFTER_PIPE)// before
	{
		// printf("the pipe is before the command\n");
		cmd->fd.in = cmd->prev->pipe[0]; // or prev->pipe[1];
		// printf("previos command is %s\n", cmd->prev->cmd_name);
	}
	else if (cmd->pipe_flag == BEFORE_PIPE)// after
	{
		// printf("the pipe is after the command\n");
		if (pipe(cmd->pipe) == -1)
			return(1);
		// printf("before\n");
		// printf("\npipe entries = [ %d | %d ]\n\n", cmd->pipe[0], cmd->pipe[1]);
		cmd->fd.out = cmd->pipe[1]; // or cmd->pipe[0]
	}
	else if (cmd->pipe_flag == BETWEEN_PIPES)// between
	{
		// printf("the pipe is between commands\n");
		if (pipe(cmd->pipe) == -1)
			return(1);
		// printf("\npipe entries = [ %d | %d ]\n\n", cmd->pipe[0], cmd->pipe[1]);
		cmd->fd.in = cmd->prev->pipe[0];
		cmd->fd.out = cmd->pipe[1];
	}
	return (0);
}

char *cmd_exist(char *cmd_name, char **path, bool *exist)
{
	int	i;
	char *tmp;

	tmp=ft_strjoin(ft_strdup("/"), cmd_name);
	
	i = 0;
	while (path[i])
	{
		cmd_name = ft_strjoin(ft_strdup(path[i]), ft_strdup(tmp));
		if (!access(cmd_name, X_OK))
		{
			*exist = true;
			// printf("full path %s\n", cmd_name);
			return (cmd_name);
		}
		i++;
	}
	return (NULL);
}


int	_execute(t_simple_cmd *cmd, t_data *data)// child process
{
	char **path = get_path_env(data->env_l);
	char **env = list_to_arr(data->env_l);
	bool exist;
	exist = false;
	cmd->cmd[0] = cmd_exist(cmd->cmd[0], path, &exist);
	// print_cmd(cmd);
	if (cmd->fd.in != 0)
	{
		dup2(cmd->fd.in, STDIN_FILENO);
		// close(cmd->fd.in);
	}
	if (cmd->fd.out != 1)
	{
		dup2(cmd->fd.out, STDOUT_FILENO);
		// close(cmd->fd.out);
	}
	if (cmd->pipe_flag == AFTER_PIPE || cmd->pipe_flag == BETWEEN_PIPES)// close unused pipes in child process
	{
		close(cmd->prev->pipe[0]);
		close(cmd->prev->pipe[1]);
		// dprintf(2, "close pipe [%d, %d] in child\n", cmd->prev->pipe[0], cmd->prev->pipe[1]);
	}
	close_all_fds(cmd);
	execve(cmd->cmd[0], cmd->cmd, env);
	printf("exeve doesn't work\n");
	exit(1);
}

int	handle_cmd(t_simple_cmd *cmd, t_data *data)
{
	int pid;
	int state;

	pid = fork();
	signal(SIGQUIT, SIG_DFL);
	if (!pid)// child
	{
		// printf("\n\033[0;32m=============================================================\033[0m\n");
		handle_here_doc(cmd);
		handle_redirections(cmd);
		if (check_builtin(cmd->cmd_name))
		{
			builtin_cmd(cmd, data, check_builtin(cmd->cmd_name));
			exit(0);
		}
		else
			_execute(cmd, data);
		// printf("\n\033[0;32m=============================================================\033[0m\n");
	}
	else// parent
	{
		// printf("\t\twaiting for child..\n");
		// printf("\tin = %d out = %d\n", cmd->fd.in , cmd->fd.out);
		if (cmd->pipe_flag == AFTER_PIPE || cmd->pipe_flag == BETWEEN_PIPES)// close unused pipes in parent process
		{
			close(cmd->prev->pipe[0]);
			close(cmd->prev->pipe[1]);
			// printf("close pipe [%d, %d] in parent\n", cmd->prev->pipe[0], cmd->prev->pipe[1]);
		}
		waitpid(pid, &state, 0);
		if (WIFSIGNALED(state))
		{
			exit_status = WTERMSIG(state);
			printf("sig ==> %d\n", exit_status);
		}
		else if (WIFEXITED(state))
		{
			exit_status = WEXITSTATUS(state);
			printf("GEN ==> %d\n", exit_status);
		}
	}
	return (0);
}

int	execute_cmd(t_simple_cmd *cmd, t_data *data)
{
	if (cmd->pipe_flag == NO_PIPE)
	{
		handle_cmd(cmd, data);
		return(1);
	}
	while (cmd)
	{
		if (check_builtin(cmd->cmd_name))
		{
			builtin_cmd(cmd, data, check_builtin(cmd->cmd_name));
			break;
		}
		handle_pipes(cmd);
		handle_cmd(cmd, data);
		// if (cmd->pipe_flag == AFTER_PIPE || cmd->pipe_flag == BETWEEN_PIPES)// close unused pipes in parent process
		// {
		// 	close(cmd->prev->pipe[0]);
		// 	close(cmd->prev->pipe[1]);
		// 	// printf("close pipe [%d, %d]\n", cmd->prev->pipe[0], cmd->prev->pipe[1]);
		// }
		cmd = cmd->next;
	}
	return (0);
}
