#include "cmd.h"

/*
	cmd_init 
	A partir d'une chaine de caractère, on cree une commande avec des redirections de fichier
	la commande sera composé de la commande, des arguments, redirection de fichier et backquotes
*/

Cmd* cmd_init(char str[]){
	Cmd* cmd=(Cmd*)leash_malloc(sizeof(Cmd));

	int i=0;

	cmd->fd_in=-1;
	cmd->fd_out=-1;
	cmd->print=1;
	cmd->nbArgs=1;
	cmd->backquoted_cmd=NULL;
	cmd->backquoted_index=-1;
	cmd->arguments=(char**)leash_malloc(sizeof(char*) * (nbWords(str)+1));
	
	char s[2]=" ";
	char* token;
	i=0;
	int file_in=0;
	int file_out=0;
	token = strtok(str, s);
	int action=0;
	int doubleQuote=0;
	int backQuote=0;
	while( token != NULL ) {
		if(i==0){
			/* Le premier parametre est le nom de la commande */
			cmd->nom=(char*)leash_malloc(sizeof(char)*(strlen(token)+1));
			strcpy(cmd->nom,token);
			cmd->arguments[0]=(char*)leash_malloc(sizeof(char)*(strlen(token)+1));
			strcpy(cmd->arguments[0],token);
			i++;
		}else{
			if(file_in==1){
				/* Si on a rencontre le char '<' */
				action=1;
				if(cmd->fd_in==-1){
					cmd->fd_in=open(token,O_RDONLY);
					if(cmd->fd_in==-1){
						printf("erreur ouverture %s\n",token);
					}
				}else{
					printf("%s ignoré, il y a déjà un fichier d'éntré\n",token );
				}
				file_in=-1;
			}
			if(file_out==1){
				/* Si on a rencontre le char '>' */
				action=1;
				if(cmd->fd_out==-1){
					cmd->fd_out=open(token,O_RDWR|O_CREAT|O_TRUNC,S_IRWXU);
					if(cmd->fd_out==-1){
						printf("erreur ouverture %s\n",token);
						break;
					}
					cmd->print=0;
				}else{
					printf("%s ignoré, il y a déjà un fichier de sortie\n",token );
				}
				file_out=-1;
			}
			if(file_out==2){
				/* Si on a rencontre les char '>>' */
				action=1;
				if(cmd->fd_out==-1){
					int file=open(token,O_RDWR|O_CREAT,S_IRWXU);
					if(file==-1){
						printf("erreur ouverture %s\n",token);
						break;
					}
					lseek(file, 0, SEEK_END);
					cmd->fd_out=file;
					cmd->print=0;
				}else{
					printf("%s ignoré, il y a déjà un fichier de sortie\n",token );
				}
				file_out=-1;
			}
			if(!strcmp("<",token)){
				/* On s'attend a avoir un fichier apres */
				if(cmd->fd_in!=-1){
					printf("Seulement un seul < ,");
				}
				action=1;
				file_in=1;
			}
			if(!strcmp(">",token)){
				/* On s'attend a avoir un fichier apres */
				if(cmd->fd_out!=-1){
					printf("Seulement un seul > ,");
				}
				action=1;
				file_out=1;
			}
			if(!strcmp(">>",token)){
				/* On s'attend a avoir un fichier apres */
				if(cmd->fd_out!=-1){
					printf("Seulement un seul >> ,");
				}
				action=1;
				file_out=2;
			}
			if(!action){
				if(token[0]=='"' && !doubleQuote){
					/* Si on rencontre une double quote, on gardera le parametre comme un seul argument */
					token=&token[1];
					doubleQuote=1;
					cmd->arguments[i]="";
				}

				if(token[0]=='`' &&!backQuote){
					/* Si on rencontre une backquote, on met la chaine dans l'attribut backquote_cmd jusqu'a la fermeture*/
					cmd->backquoted=1;
					token=&token[1];
					backQuote=1;
					cmd->arguments[i]="";
				}
				
				if(doubleQuote){
					/* On concatene dans le meme parametre pour les double quote */
					char* prev=cmd->arguments[i];
					char* new=(char*)leash_malloc(strlen(token)+strlen(prev)+2);
					memset(new,0,strlen(token)+strlen(prev)+2);
					if(strlen(prev)>0){
						strcpy(new,prev);
						strcat(new," ");
					}
					strcat(new,token);
					cmd->arguments[i]=new;
					if(token[strlen(token)-1]=='"'){
						cmd->arguments[i][strlen(cmd->arguments[i])-1]='\0';
						doubleQuote=0;
						cmd->nbArgs++;
						i++;
					}

				}else if(backQuote){
					/* On concatene dans le meme parametre pour les back quote */
					char* prev=cmd->arguments[i];
					char* new=(char*)leash_malloc(strlen(token)+strlen(prev)+2);
					memset(new,0,strlen(token)+strlen(prev)+2);
					if(strlen(prev)>0){
						strcpy(new,prev);
						strcat(new," ");
					}
					strcat(new,token);
					cmd->arguments[i]=new;
					if(token[strlen(token)-1]=='`'){
						cmd->arguments[i][strlen(cmd->arguments[i])-1]='\0';
						cmd->backquoted_cmd=cmd->arguments[i];
						cmd->backquoted_index=i;
						cmd->arguments[i]=NULL;
						backQuote=0;
						cmd->nbArgs++;
						i++;
					}

				}else{
					/* Un simple argument */
					cmd->arguments[i]=(char*)leash_malloc(strlen(token)+1);
					strcpy(cmd->arguments[i],token);
					cmd->nbArgs++;
					i++;
				}
				
			}else{
				action=0;
			}

		}
		token = strtok(NULL, s);
	}
	cmd->arguments[i]=(char*)NULL;

	cmd->pid=-1;
	cmd->result=-1;
	cmd->backquoted=0;


	return cmd;
}




void cmd_dest(Cmd* cmd){
	if(cmd!=NULL){
		free(cmd->nom);
		int i=0;
		for(i=0;i<cmd->nbArgs;i++){
			free(cmd->arguments[i]);
		}
		free(cmd->backquoted_cmd);
		free(cmd->arguments);
		leash_close(cmd->fd_in);
		leash_close(cmd->fd_out);
		free(cmd);
	}
}

void cmd_print(Cmd* cmd){
	printf("%s  pid : %d\n",cmd->nom,cmd->pid );
	int i=0;
	for(i=0;i<cmd->nbArgs;i++){
		printf("\t%s\n", cmd->arguments[i]);
	}
	printf("in : %d, out : %d\n", cmd->fd_in,cmd->fd_out);
	printf("backquoted_cmd [%d]: %s\n",cmd->backquoted_index,cmd->backquoted_cmd );
	printf("res : %d\n",cmd->result );
}



/* 
	cmd_exec_backquoted
	permet d'executer une commande donnee par chaine de caractere
	en tant que commande dans des backquote

	La difference avec une commande normal est que le resultat est stocke dans un tableau
	qui sera insere dans le tableau d'argument de la commande principale

*/

char** cmd_exec_backquoted(char* strcmd){
	Cmd* cmd = cmd_init(strcmd);
	cmd->print=0;
	int fd[2];
	pipe(fd);
	cmd->fd_out=fd[1];

	cmd_exec(cmd);
	leash_close(fd[1]);

	List* liste = liste_init();

	int lu=0;
	char c;
	int len=0;
	int size=100;
	char* str=(char*)malloc(size);
	memset(str,0,size+1);
	while((lu=read(fd[0],&c,1))>0){
		if(c=='\n'){
			/* on ajoute l'argument dans une nouvelle case du tableau
			lorsqu'on rencontre \n */
			liste_add_last(liste,(void*)strdup(str));
			memset(str,0,size);
			len=0;
		}else{
			if(len==size-1){
				size=size*2;
				str=(char*)realloc(str,size);
			}
			str[len]=c;
			str[len+1]='\0';
			len++;
		}
	}
	free(str);

	char** res=(char**)malloc(sizeof(char*)*(liste->size+1));
	int i=0;
	for(i=0;i<liste->size;i++){
		res[i]=strdup((char*)liste_get(liste,i)->object);
	}
	res[liste->size]=NULL;

	cmd_dest(cmd);

	return res;
}


int fd_out_Y[2];
void handlerchld_child(int sig){
	leash_close(fd_out_Y[1]);
}

static int pid;
void handlerchld(int sig){
	kill(pid,SIGTERM);
	leash_close(fd_out_Y[1]);
}

void cmd_exec(Cmd* cmd){
	/* Mise en place des signal pour ctrl-C et ctrl-\ */
	struct sigaction nvt,old;	
	memset(&nvt,0,sizeof(nvt));

	nvt.sa_handler = &handlerchld;
	sigaction(SIGINT,&nvt,&old);
	signal(SIGINT,&handlerchld);

	struct sigaction nvt2,old2;	
	memset(&nvt2,0,sizeof(nvt2));

	nvt2.sa_handler = &handlerchld_child;
	sigaction(SIGCHLD,&nvt2,&old2);
	signal(SIGCHLD,&handlerchld_child);

	int res=0;

	pipe(fd_out_Y);

	/* on cree le fils pour executer la commande */
	pid=fork();
	if(pid==-1){
		perror("Error fork execSimple");
		exit(EXIT_FAILURE);
	}
	if(!pid){

		/* FILS */

		int res=0;
		if(cmd->fd_in>=0){
			/* on branche le fichier d'entree sur la commande */
			dup2(cmd->fd_in,0);
		}
		if(cmd->fd_out>=0){
			/* on branche le fichier de sortie sur la commande ou le pipe de sortie*/
			dup2(1,2);
			if(cmd->print){
				/* 
				Si on affiche du texte (il n'y a pas de redirection de fichier > ou >>
				alors on ecrit dans un pipe qui sera lu pendant l'execution puis comparé plus tard
				*/
				dup2(fd_out_Y[1],1);
			}else{
				/* Sinon on met la sortie sur le fichier donné en parametre*/
				dup2(cmd->fd_out,1);
			}
		}


		/* GLOBBING */
		glob_t globbuf;

		int i=0;
		int flags=GLOB_NOCHECK;

		/* on essai d'appliquer le globbing sur chaque argument
		grace au flag GLOB_NOCHECK on prend en compte les parametres non globbable*/
		for(i=0;i<cmd->nbArgs;i++){
			flags |= (i >0 ? GLOB_APPEND : 0);
			glob(cmd->arguments[i], flags  , NULL, &globbuf);
		}
		/* Ensuite on met le resultat dans la commande puis on recompte le nombre d'argument */
		cmd->arguments=globbuf.gl_pathv;
		int nb=0;
		while(cmd->arguments[nb]!=NULL){
			nb++;
		}
		cmd->nbArgs=nb;



		/* BACKQUOTE */
		if(cmd->backquoted_index!=-1){
			/* on execute puis recupere le resultat de la commande en backquote */
			char** back = cmd_exec_backquoted(cmd->backquoted_cmd);
			int nbBack=0;				
			while(back[nbBack]!=NULL){
				nbBack++;
			}

			/* ensuite on place le resultat au bon endroit dans le tableau d'argument
			au meme endroit que la commande backquote*/
			char** args = (char**)leash_malloc(sizeof(char*)*(cmd->nbArgs+nbBack));
			int i=0;
			int include=0;
			int indexArg=0;
			int indexBack=0;
			for(i=0;i<cmd->nbArgs+nbBack;i++){
				if(i==cmd->backquoted_index){
					include=1;
					indexArg++;
				}
				if(i==cmd->backquoted_index+nbBack){
					include=0;
				}
				if(include){
					args[i]=back[indexBack];
					indexBack++;
				}else{
					args[i]=cmd->arguments[indexArg];
					indexArg++;
				}
			}
			args[i]=NULL;

			/* Puis on met a jour les argument de la commande*/
			cmd->arguments=args;
			cmd->nbArgs=cmd->nbArgs+nbBack;
				
		}	

		if(strcmp("pwd",cmd->nom) && strcmp("exit",cmd->nom) && strcmp("cd",cmd->nom) && strcmp("about",cmd->nom)){
			/* Si ce n'est pas une commande qui est implemente dans ce projet
			alors on fait un exec*/
			execvp(cmd->nom,cmd->arguments);
			return;
		}else{
			/* sinon on appele la commande implemente par nos soins*/
			if(strcmp("pwd",cmd->nom) == 0){
				res=command_pwd();
			}else if(strcmp("cd",cmd->nom) == 0){
				res=command_cd(cmd->arguments[1]);
			}else if(strcmp("about",cmd->nom) == 0){
				char* arg=" ";
				if(cmd->nbArgs>1){
					arg=cmd->arguments[1];
				}
				res=command_about(arg);
			}
			exit(res);
		}

	}else{

		/* PERE */
		/* si le programme doit afficher du texte a l'ecran, alors on l'affiche*/
		if(cmd->print){
			char buff[9];
			buff[8]='\0';
			memset(buff,0,9);
			int lu=0;
			/* on lit le tube de sortie, on l'affiche
			puis on le remet dans un tube qui sera lu pour verifier le resultat*/
			while((lu=read(fd_out_Y[0],&buff,8))){
				buff[lu]='\0';
				printf("%s",buff);
				fflush(stdout);
				write(cmd->fd_out,buff,lu);
				memset(buff,0,9);
			}
			leash_close(fd_out_Y[0]);
		}

		/* On attend le fils puis on verifie le resultat */
		waitpid(pid,&(cmd->result),0);
		leash_close(cmd->fd_in);
		if(cmd->result==0 && strcmp("cd",cmd->nom)==0){
			command_cd(cmd->arguments[1]);
		}else if(strcmp("exit",cmd->nom) == 0){
			res=command_exit();
		}
		
	}

	res=cmd->result;
}




