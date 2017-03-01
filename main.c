#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

enum {
	MaxLigne = 1024,              // longueur max d'une ligne de commandes
	MaxMot = MaxLigne / 2,        // nbre max de mot dans la ligne
	MaxDirs = 100,		// nbre max de repertoire dans PATH
	MaxPathLength = 512,		// longueur max d'un nom de fichier
	MaxCommandes = 100,
};

struct task
{
	int pid;
	char *cmd;
	int state;
	int finished;
};
typedef struct task task;
task **task_list;

int decouper(char *, char *, char **, int);
int clean_redir(char *, char *, char *);
void remove_task(int pid, int state);
void clear_task();
int add_task(int pid, char *cmd);
void print_task();

# define PROMPT "mishell &"

#define MAX_TASK 30

int current_fg = -1;

void chld_handler(int sig)
{
	signal(SIGCHLD, chld_handler);
	int state;
	int pid = waitpid(-1, &state, WNOHANG);
	if (pid > 0)
    {
    	remove_task(pid, state);
         //error("error handler ");
        return ;
    }
    
}

void int_handler(int sig)
{
	//printf("curr %d\n", current_fg);
	if(current_fg > 0)
	{
		if(kill(current_fg, SIGINT) < 0)
			perror("kill");

	}
	else
		printf(" No fg task found. Type exit to leave.");
	
	signal(SIGINT, int_handler);

	printf("\n"PROMPT);
	fflush(stdout);
}

void stop_handler(int sig)
{
	printf("test\n");
	if(current_fg > 0)
	{
		if(kill(current_fg, SIGTSTP) < 0)
			perror("kill");
	}
	else
		printf("\nNo fg task found.\n");
	
	signal(SIGTSTP, stop_handler);

	printf("\n"PROMPT);
	fflush(stdout);

}

void remove_task(int pid, int state)
{
	int i;
	for(i = 0; i < MAX_TASK; i++)
	{
		task *t = *(task_list + i);
		if(t && t->pid == pid)
		{
			t->finished = 1;
			t->state = state;
		}
	}
}

void clear_task()
{
	int i;
	for(i = 0; i < MAX_TASK; i++)
	{
		task *t = *(task_list + i);
		if(t && t->finished)
		{
			printf("~~| Finished task %d with state %d.\n~~| PID : %d - CMD : %s.\n", i, t->state, t->pid, t->cmd);
			free(t->cmd);
			free(*(task_list + i));
			*(task_list + i) = NULL;
		}
	}
}

int add_task(int pid, char *cmd)
{
	task *t = NULL;
	t = malloc(sizeof(task));
	if(!t)
	{
		printf("add_task : error\n");
		return -1;
	}
	t->pid = pid;
	t->cmd = strdup(cmd);
	t->finished = 0;

	int i;
	for(i = 0; i < MAX_TASK; i++)
	{
		if(*(task_list + i) == NULL)
		{
			*(task_list + i) = t;
			printf("~~| New task %d\n~~| PID : %d. CMD : %s\n", i, t->pid, t->cmd);
			break;
		}
	}

	if(i == MAX_TASK)
		return -1;
	return i;
}

void print_task()
{
	int i;
	for(i = 0; i < MAX_TASK; i++)
	{
		task *t = *(task_list + i);
		if(t != NULL)
			printf("[%d] - %s (pid %d, state %d)\n", i, t->cmd, t->pid, t->state);
	}
}

int main(int argc, char * argv[])
{
	//printf("group : %d. pid : %d\n", getpgid(getpid()), getpid());
	task_list = malloc(sizeof(task*) * MAX_TASK);
	{
		int i;
		for(i = 0; i < MAX_TASK; i++)
		{
			*(task_list + i) = NULL;
		}
	}


	char ligne[MaxLigne];
	char *ligne_copy;
	char pathname[MaxPathLength];
	char * mot[MaxMot];
	char * dirs[MaxDirs];
	char *commandes[MaxCommandes];
	int i_cmd, i_dir, pid, nb_cmds;
	clock_t current_time;

	/* Decouper UNE COPIE de PATH en repertoires */
	decouper(strdup(getenv("PATH")), ":", dirs, MaxDirs);


	/* Lire et traiter chaque ligne de commande */
	int i;
	int fg;
	char *task_cmd;

	signal(SIGCHLD, chld_handler);
	signal(SIGINT, int_handler);
	signal(SIGTSTP, stop_handler);

	int before_pipe = -1;

	while(1)
	{

		clear_task();
		printf(PROMPT);
		fflush(stdout);
		if(!fgets(ligne, sizeof ligne, stdin))
			break;

		nb_cmds = decouper(ligne, ";&|", commandes, MaxCommandes);
		for(i = 0; i < nb_cmds; i++)
		{
			current_fg = -1;
			task_cmd = strdup(commandes[i]);
			if(*(task_cmd + strlen(task_cmd) - 1) == '\n')
			{
				*(task_cmd + strlen(task_cmd) - 1) = '\0';
			}

			//printf("test : %s\n", task_cmd);
			
			//printf("apres manip\n");

			// Determiner si le processus doit etre lance en fond de tache
			
			int separator = ';';
			if(ligne[commandes[i] - commandes[0] + strlen(commandes[i])] == '&')
				separator = '&';
			else if(ligne[commandes[i] - commandes[0] + strlen(commandes[i])] == '|')
				separator = '|';
			// Nothing ? Separator is ';'
			if(separator == '|' && !commandes[i + 1])
				separator = ';';
			
			char *clean_cmd = malloc(sizeof(char) * (strlen(commandes[i]) + 1));
			char *clean_file= malloc(sizeof(char) * (strlen(commandes[i]) + 1));
			
			int redir = clean_redir(commandes[i], clean_cmd, clean_file);
			//printf("redir : %d - cmd : %s - file :%s\n", redir, clean_cmd, clean_file);

			// now don't use commandes[i], use clean_cmd
			// if there is a redirection (0, 1 or 2) use clean_file

			int nbmots = decouper(clean_cmd, " \t\n", mot, MaxMot);
			//printf("test\n");

			if(!nbmots)
			{
			}
	
			else if(!strcmp(mot[0], "cd"))
			{
				char *path;
				if(nbmots < 2)
					path = strdup(getenv("HOME"));
				else
					path = strdup(mot[1]);
				if(chdir(path) < 0)
					perror("chdir");
			}
			else if(!strcmp(mot[0], "jobs"))
				print_task();
			else if(!strcmp(mot[0], "exit"))
				exit(1);
			else if(!strcmp(mot[0], "jkillall"))
			{
				int j;
				for(j = 0; j < MAX_TASK; j++)
				{
					task *t = *(task_list + j);
					if(t)
					{
						if(kill(t->pid, SIGINT) < 0)
							perror("kill");
					}
				}
			}

			else
			{
				current_fg = -1;
				int pfd[2];
				if(separator == '|')
				{
					// the pipe needs to be created before fork
					if(pipe(pfd) == -1)
					{
						perror("pipe");
						continue;
					}
					before_pipe = pfd[0];
					
				}
				pid = fork();
				
				if(pid > 0)
				{

					if(separator == ';')
					{
						current_fg = pid;
						int statee;
						if(waitpid(pid, &statee, 0 | WUNTRACED) < 0)
						{
							perror("waitpid");
						}
						//printf("Leave pid %d with state %d\n", pid, statee);
						current_fg = -1;
						//while(wait(NULL) != pid);
						//remove_task(pid, state);
					}
					else if(separator == '&')
					{
						add_task(pid, strdup(task_cmd));
					}
					
				}
				
				else if(pid == 0)
				{
					setpgid(getpid(), getpid());
					
					signal(SIGINT, SIG_DFL);
					signal(SIGCHLD, SIG_DFL);
					signal(SIGTSTP, SIG_DFL);

					if(before_pipe != -1)
					{
						//close(0);
						dup2(before_pipe, 0);
					}

					// redirections en fonction de redir

					if(redir == 1)
					{
						//redirection stdout
						int out = open(clean_file, O_CREAT | O_WRONLY | O_TRUNC);
						if(out < 0)
						{
							perror("open");
							return -1;
						}
						dup2(out, 1);
					}
					else if(redir == 2)
					{
						int out = open(clean_file, O_WRONLY|O_CREAT|O_APPEND);
						if(out < 0)
						{
							perror("open");
							return -1;
						}
						dup2(out, 1);
					}
					else if(redir == 0)
					{
						int in = open(clean_file, O_RDONLY);
						if(in < 0)
						{
							perror("open");
							return -1;
						}
						dup2(in, 0);
					}


					if(separator == '&')
					{
						// redirect stdin to null
						int devnull = open("/dev/null", O_RDONLY);
    					if (devnull < 0)
    					{
    						perror("open");
    					}
    					else
						{
							dup2(devnull, 0);
						}
					}
					else if(separator == '|')
					{
						// supprimer la lecture
						//close(pfd[0]);
						dup2(pfd[1], 1);
					}

					for(i_dir = 0; dirs[i_dir] != NULL; i_dir++)
					{
						snprintf(pathname, sizeof pathname, "%s/%s", dirs[i_dir], mot[0]);
						//printf("%s %s\n", pathname, mot);
						execv(pathname, mot);
					}
					fprintf(stderr, "%s: not found\n", mot[0]);
					exit(1);
					
				}

				else
				{
					perror("fork");
				}
			}
		}
	}

	printf("Bye\n");
	return 0;
}
