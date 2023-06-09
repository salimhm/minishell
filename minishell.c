/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   minishell.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: shmimi <shmimi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/19 22:59:20 by abouram           #+#    #+#             */
/*   Updated: 2023/07/13 23:17:40 by shmimi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

int g_exit_status;
#include "minishell.h"


int get_num_pipes(t_table *list)
{
	t_table *current = list;
	int i = 0;

	while (current)
	{
		i += current->pip;
		current = current->next;
	}

	return i;
}

int magic(t_table *list, t_list *my_env, char **env)
{

	t_pipes_n_redirection *pipes_n_redirection = ft_calloc(sizeof(t_pipes_n_redirection), 1);

	pipes_n_redirection->env2d = list_to_double_pointer(my_env);
	pipes_n_redirection->path = get_path(pipes_n_redirection->env2d);

	pipes_n_redirection->num_pipes = get_num_pipes(list);
	pipes_n_redirection->pids = ft_calloc(sizeof(int), pipes_n_redirection->num_pipes + 1);

	t_table *current_heredoc = list;
	pipes_n_redirection->cmd = NULL;
	pipes_n_redirection->args = NULL;
	pipes_n_redirection->filename = NULL;

	// pipes_n_redirection->filemames = malloc(sizeof(char *) * pipes_n_redirection->num_pipes + 1);
	int x = 0;
	int k = 0;
	t_table *current = list;
	while (current)
	{
		if (current->cmd)
		{
			free(pipes_n_redirection->cmd);
			free2d(pipes_n_redirection->args);
			pipes_n_redirection->cmd = check_valid_cmd(current->cmd, pipes_n_redirection->path);
			pipes_n_redirection->args = copy_args_to_2d(current->cmd, current->arg);
		}

		/************************* Handle << redirection (Heredoc) ********************/
		while (current_heredoc)
		{
			pipes_n_redirection->pos_redirection = get_pos_redirection(current->redirection->type, "<<");
			while (x < current_heredoc->redirection->heredoc)
			{
				pipes_n_redirection->pos_redirection_v2 = get_pos_redirection_v2(pipes_n_redirection->pos_redirection, current_heredoc->redirection->type, "<<");
				pipes_n_redirection->buffer = ft_calloc(1, 1);
				if (current_heredoc->redirection->file[pipes_n_redirection->pos_redirection_v2])
				{
					free(pipes_n_redirection->filename);
					pipes_n_redirection->filename = ft_strjoin("/tmp/", current_heredoc->redirection->file[pipes_n_redirection->pos_redirection_v2]);
				}
				if (pipes_n_redirection->tmp)
					close(pipes_n_redirection->tmp);

				if (pipes_n_redirection->tmp)
					close(pipes_n_redirection->tmp);
				if (pipes_n_redirection->in)
					close(pipes_n_redirection->in);

				pipes_n_redirection->tmp = open(pipes_n_redirection->filename, O_CREAT | O_RDWR, 0666);
				pipes_n_redirection->in = pipes_n_redirection->tmp;
				while (1)
				{
					pipes_n_redirection->input = readline("> ");
					if (ft_strncmp(pipes_n_redirection->input, current_heredoc->redirection->file[pipes_n_redirection->pos_redirection_v2], ft_strlen(current_heredoc->redirection->file[pipes_n_redirection->pos_redirection_v2]) + 1) == 0)
					{
						write(pipes_n_redirection->in, pipes_n_redirection->buffer, ft_strlen(pipes_n_redirection->buffer));
						free(pipes_n_redirection->input);
						free(pipes_n_redirection->buffer);
						pipes_n_redirection->pos_redirection++;
						pipes_n_redirection->flag = 1;
						break;
					}
					pipes_n_redirection->tmp_buffer = ft_strjoin(pipes_n_redirection->buffer, pipes_n_redirection->input);
					free(pipes_n_redirection->buffer);
					pipes_n_redirection->buffer = ft_strjoin(pipes_n_redirection->tmp_buffer, "\n");
					free(pipes_n_redirection->tmp_buffer);
					free(pipes_n_redirection->input);
				}
				x++;
			}
			x = 0;
			current_heredoc = current_heredoc->next;
		}
		/************************* End << Heredoc ************************************/
		if (pipes_n_redirection->flag && current->cmd && !current->pip)
		{
			pipes_n_redirection->in = open(pipes_n_redirection->filename, O_RDONLY, 0666);
		}
		/***************************** PIPING *************************/
		pipe(pipes_n_redirection->pipefds);
		pipes_n_redirection->pid = fork();
		if (pipes_n_redirection->pid == 0)
		{
			child(current, pipes_n_redirection, my_env, g_exit_status);
		}
		else
		{
			parent(current, pipes_n_redirection, my_env, g_exit_status);
		}
		// Get all pids
		pipes_n_redirection->pids[k] = pipes_n_redirection->pid;
		k++;

		current = current->next;
	}

	if (pipes_n_redirection->tmp)
		close(pipes_n_redirection->tmp);
	if (pipes_n_redirection->in)
		close(pipes_n_redirection->in);

	free2d(pipes_n_redirection->env2d);
	free2d(pipes_n_redirection->path);

	if (pipes_n_redirection->cmd && pipes_n_redirection->args)
	{
		free2d(pipes_n_redirection->args);
		free(pipes_n_redirection->cmd);
	}
	if (pipes_n_redirection->pipefds[0])
	{
		close(pipes_n_redirection->pipefds[0]);
		close(pipes_n_redirection->pipefds[1]);
	}
	if (pipes_n_redirection->flag)
	{
		free(pipes_n_redirection->filename);
		unlink(pipes_n_redirection->filename);
	}
	k = 0;
	// while (pipes_n_redirection->filemames[k])
	// {
	// 	unlink(pipes_n_redirection->filename);
	// 	k++;
	// }
	k = 0;
	while (k < pipes_n_redirection->num_pipes + 1)
	{
		waitpid(pipes_n_redirection->pids[k], &g_exit_status, 0);
		k++;
	}
	free(pipes_n_redirection->pids);
	g_exit_status = WEXITSTATUS(g_exit_status);

	free(pipes_n_redirection);
	// while (1)
	// 	;
	return 0;
}

int get_pos_redirection_v2(int start, char **redirection, char *redirection_type)
{
	while (redirection[start])
	{
		if (ft_strncmp(redirection[start], redirection_type, ft_strlen(redirection_type) + 1) == 0)
			return start;
		start++;
	}
	return 0;
}

int get_pos_redirection(char **redirection, char *redirection_type)
{
	int i = 0;
	while (redirection[i])
	{
		if (ft_strncmp(redirection[i], redirection_type, ft_strlen(redirection_type) + 1) == 0)
			return i;
		i++;
	}
	return 0;
}

int is_builtin(char *builtin)
{
	char *built_ins[] = {"env", "export", "echo", "cd", "pwd", "unset", "exit"};
	int i = 0;

	while (i < 7)
	{
		if (ft_strncmp(builtin, built_ins[i], ft_strlen(builtin) + 1) == 0)
		{
			// printf("\n--builtin %s\n", builtin);
			return 1;
		}
		i++;
	}
	return 0;
}
void	parser_arg(char *input, char **env, t_list *my_env)
{
	t_table	*final_list;
	char	**str;
	char	**s;
	t_myarg	*arg;

	arg = malloc(1 * sizeof(t_myarg));
	arg->quote = 0;
	arg->num_alloc = 0;
	here_doc_expaand(input, arg);
	account_quote(input, arg);
	arg->num_alloc = num_alloc_str(input);
	if (arg->quote % 2 == 1)
		printf("%s\n",
			"minishell: syntax error near unexpected token `\"' or `\''");
	else
	{
		str = ft_split(input, '\"');
		s = ft_calloc(sizeof(char *), arg->num_alloc + 1);
		arg->x = 0;
		arg->i = 0;
		arg->index = 0;
		s = get_token_from_str(str, s, arg);
		arg->final_expand = expand(s, my_env, arg->num_alloc, arg);
		arg->final_expand = clean_expand(arg->final_expand, "\3\4\5\6", arg);
		final_list = final_addition(arg->final_expand);
		if (final_list == NULL)
			return;
		final_list->exp_exit = arg->exp_exit; // *******this for expand the exit status if 1 don't (if 0 expand) *******//
		final_list->exp_heredoc = arg->exp_heredoc;// *******this for expand inside heredoc status if 1 don't (if 0 expand)*******//
		magic(final_list, my_env, env);
	}
	int x = 0;
		while (final_list)
		{
			printf("********************* BEGIN ***************************\n");
			printf("_____CMD_____=..%s\n\n", final_list->cmd);
			x = 0;
			while (final_list->arg[x])
				printf("_____ARG_____=..%s\n\n", final_list->arg[x++]);
			printf("_____PIPE_____=..%s\n\n", final_list->redirection->pipe);
			x = 0;
			while (final_list->redirection->type[x])
				printf("_____TYPE_REDI_____=..%s\n\n", final_list->redirection->type[x++]);
			x = 0;
			while (final_list->redirection->file[x])
				printf("_____FILE_____=..%s\n\n", final_list->redirection->file[x++]);
			final_list = final_list->next;
		}
}

void	sig_int()
{
	// rl_replace_line("", 0);
	printf("\n");
	rl_on_new_line();
	rl_redisplay();
	return ;
}

int main(int ac, char **av, char **env)
{
	char *input;

	(void)av;
	t_list *my_env = NULL;
	if (ac != 1)
		return (1);
	my_env = get_env(env);
	g_exit_status = 0;
	while (1)
	{
		signal(SIGINT, sig_int);
		signal(SIGQUIT, SIG_IGN);
		input = readline("minishell$ ");
		add_history(input);
		if (input)
			parser_arg(input, env, my_env);
		free(input);
		if (!input)
			return (0);
	}
	return (0);
}
