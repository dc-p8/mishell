# include <stdio.h>
# include <string.h>

/* decouper  --  decouper une chaine en mots */
int decouper(char * ligne, char * separ, char * mot[], int maxmot)
{

	int i;

	mot[0] = strtok(strdup(ligne), separ);
	for(i = 1; mot[i - 1] != NULL; i++){
		if (i == maxmot){
			fprintf(stderr, "Erreur dans la fonction decouper: trop de mots\n");
			mot[i - 1] = NULL;
			break;
		}
		mot[i] = strtok(NULL, separ);
	}
	return i - 1;
}

int clean_redir(char *src, char *cmd, char *file)
{
	memset(cmd, 0, strlen(src) + 1);
	memset(file, 0, strlen(src) + 1);
	int redir = -1;
	char c;
	int chpos;
	int j;
	for(chpos = 0; chpos < strlen(src); chpos++)
	{
		c = *(src + chpos);
		if(c == '>')
		{
			redir = 1;
			if(chpos < strlen(src) + 1 && *(src + chpos + 1) == '>')
			{
				chpos++;
				redir = 2;
			}
			break;

		}
		else if(c == '<')
		{
			redir = 0;
			break;
		}
		*(cmd + chpos) = c;
	}
	chpos++;
	if(redir >= 0)
	{
		int cc = 0;
		while(*(src + chpos) != '\0')
		{
			*(file + cc) = *(src + chpos);
			cc++;
			chpos++;
		}
	}
	
	while(cmd[0] == ' ' || cmd[0] == '\t' || cmd[0] == '\n')
	{
		j = 0;
		while(cmd[j + 1] != '\0')
		{
			*(cmd + j) = *(cmd + j + 1);
			j++;
		}
		cmd[j] = '\0';
	}
	while(cmd[strlen(cmd) - 1] == ' ' || cmd[strlen(cmd) - 1] == '\t' || cmd[strlen(cmd) - 1] == '\n')
	{
		cmd[strlen(cmd) - 1] = '\0';
	}
	while(file[0] == ' ' || file[0] == '\t' || file[0] == '\n')
	{
		j = 0;
		while(file[j + 1] != '\0')
		{
			*(file + j) = *(file + j + 1);
			j++;
		}
		file[j] = '\0';
	}
	while(file[strlen(file) - 1] == ' ' || file[strlen(file) - 1] == '\t' || file[strlen(file) - 1] == '\n')
	{
		file[strlen(file) - 1] = '\0';
	}

	return redir;
}