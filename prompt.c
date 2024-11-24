/* * * * * * * * * * * * * * * * * * * * * * * * * *
 *						   *
 *   	   IOANNIS CHATZIANTONIOU CSD5193	   *
 *						   *
 * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define CMD_SIZE 1024
#define LINE_DEL " \n\t"
#define ARR_SIZE 128

char **split_cmd(char *);

void remove_space(char *);

void redirect(char **,int);

void execute_pipes(char **,int);

void handleEcho(char *);

void createVar(char *);

void print_prompt(void);

int is_unix_cmd(const char *);

void execute_cmd(char **);

char ** split_cmd_semi(char *);

void remove_space(char *);

void shell(void);

/* my global variables */

struct var{
	char *name;
	char *value;
};

struct var variables[ARR_SIZE];
int index_pub=0;

/* handles the redirection of command input or output to files */

void redirect(char ** buf,int mode){
	
	int i=0, fd;
    	char *argv[ARR_SIZE];

    	remove_space(buf[1]);  /* remove spaces from name of the file used for redirection */
    	
	/* tokenizing based on space */

	char *token;
    	char *command = buf[0];
    	token = strtok(command, " ");
    	
    	while (token != NULL) {
        	argv[i] = malloc(strlen(token) + 1);  /* memory allocation for the command arguments */
        	strcpy(argv[i], token);                 /* copy token to the array */

        	remove_space(argv[i]);              /* remove spaces from arguments */

        	token = strtok(NULL, " "); 		/* take the next token*/
  		i++;		
    	}
    	argv[i] = NULL;  /* terminate the last position of array */

	/* fork the parent process */

    	pid_t pid = fork();  

    	if (pid == 0) {  /* we are in chlid process */

		/*	O_RDONLY : read only permission 
		 *	O_WRONLY : write only permission
		 *	O_TRUNC  : delete all stored data of the existed file
		 *	O_CREAT  : create a new file if there is not
		 *	O_APPEND : writed at the end of the file
		 */

        	if (mode == 1) {
            		
                	fd = open(buf[1], O_RDONLY, 0666);
                		
		}else if ( mode == 0 ){
            		
                	fd = open(buf[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                		
		}else if (mode == -1){
            		
                	fd = open(buf[1], O_WRONLY | O_CREAT | O_APPEND, 0666);
                		
		}else      		
                		return;  /* something is wrong */
      
		
        	if (fd < 0) {
            		perror("cannot open the file\n");
            		exit(EXIT_FAILURE);  /* failure opening the file */
        	}

        	/* redirects standard input or output to another opened file descriptor ( fd variable )*/
        	
		if (mode == 1) {	
                		
				dup2(fd,0);
                		
		}else if( mode == 0){
				
				dup2(fd,1);
				
		}else if( mode == -1){
            		
               			dup2(fd,1);
               			
		}else
                		return;
        	

        	close(fd);  /* close file descriptor */

        	execvp(argv[0], argv);   /* executes command */
        	perror("ERROR ON EXECUTION"); /* if command does not exist or failed to executed */
        	exit(EXIT_FAILURE);  

    	} else if (pid > 0) {

        	wait(NULL);  /* the parent process waits the child to ends */

    	} else {

        	perror("fork fail"); /* child process failed fork */
    	}

	return;
}

/* fd[i][0]:read-end pipe
 * fd[i][1]:write-end pipe
 * function for multi pipes execution
 * with arguments : buffer(commands)
 * and cmds_num(commands number)
 * */

void execute_pipes(char ** buffer,int cmds_num){

	if(cmds_num > 16) return; /* if command number is greater than 16 return */

    	int fd[16][2];	/* each one has 2 file descriptors for read and write*/
    	
	int i;		/* counter for loops */

	char **argv;	/* to store the commands and their arguments*/

	/* for all commands */

    	for(i = 0; i < cmds_num; i++) {
        	
        	argv = split_cmd(buffer[i]); /* split the command to tokens */

		/* we make a pipe for every command except the last one */

        	if(i != cmds_num-1) {
            		if(pipe(fd[i]) < 0) {
                		perror("Error: Pipe Creation\n");
                		return;
            		}
      		}

        	if(fork() == 0) { /* creates a child process and checks if we are in child process */
            		
			if(i != cmds_num-1) {	/* if we are not to the last command then: */

                		dup2(fd[i][1], 1); /* dup2 redirects the stdout of the current process to write to the write-end of the pipe */
                		close(fd[i][0]); /* close pipes that we dont use fd for read-end*/
                		close(fd[i][1]); /* fd for write-end*/
            		}

            		if(i != 0) {	/* if this is not the first command then: */

                		dup2(fd[i-1][0], 0); /* Redirects the stdin of the current process to the read-end of the previous pipe */
                		close(fd[i-1][1]); /* close again the pipes and fd for write-end*/
                		close(fd[i-1][0]); /* fd for read-end*/
            		}

            		/* if execvp fails perror is printed 
			 * and the child process is terminated with exit(1)
			 * */
            		execvp(argv[0], argv);
            		perror("invalid command\n");
            		exit(1); 
        	}

        	/* in parent process we close the ends of the previous pipe */
        	if(i != 0) {
            		close(fd[i-1][0]);
            		close(fd[i-1][1]);
        	}

        	wait(NULL); /* parent process waits for child to ends before moves to the next process*/
    	}

	return;
}

/* here we handling echo function and print the desired message */

void handleEcho(char* cmd) {
    	
    	cmd = cmd + 5;  /* echo override */
	int i;int found;

	/* replacement of \n to ' ' space characters */

	for (int i = 0; i < strlen(cmd); i++) 
        	if (cmd[i] == '\n') 
            		cmd[i] = ' ';  
        
	/* starts the loop */

    	while (*cmd != '\0') {
        	if (*cmd == '$') {

			cmd++;	/* to pass the $ character */
            
            		char var_name[64]; /* string to put the variable name on it */
            		
			i = 0; /* we copy the variable name from cmd to var_name */
			while(cmd[i] != ' ' && cmd[i] != '\0' && cmd[i] != '\n' && cmd[i] != '$' && cmd[i] != ';' && i<64){
				var_name[i]=*(cmd+i);	
				i++;
			}
		
			//cmd[i]=' ';
            		var_name[i] = '\0';  /* add string terminal character */

            		/* search if this variable name exeists in the table */
            		found = 0;
            		for (int j = 0; j < index_pub; j++) {
                		if (strcmp(variables[j].name, var_name) == 0) {
                    		
					/* paste the value of the var_name */	

					printf("%s", variables[j].value);
                    			found = 1;
					cmd=cmd+strlen(var_name); /* we move the cmd pointer to the next string after $var_name */
					
					for(int k=0;k<strlen(var_name);k++)
						var_name[k]='\0';
					//cmd=cmd+sizeof(var_name);
                    			break;
                		}
            		}
            		if (!found){
                		/* if does not exist just print the name as it is */
                		printf("$%s ", var_name);
				cmd=cmd+strlen(var_name)+1;	
			}
            	}else {   	
			/* string does not have $ and just print the characters */	
            		printf("%c", *cmd);
            		cmd++;
        	}
	}
	printf("\n");
}

/* initialization of the global array variables */

void createVar(char *cmd){
	char * pos_equal = strchr(cmd, '='); /* position of equal sign */
	
	size_t len_name = pos_equal - cmd; /* length of name variable */

	variables[index_pub].name = malloc(len_name+1); /* +1 for \n */

	variables[index_pub].value = strdup(pos_equal + 1); /* copy the part of the string after = */

	int len = strlen(variables[index_pub].value); /* we have to ensure that our string has \0 and not \n */

	if (variables[index_pub].value[len-1] == '\n') 
		variables[index_pub].value[len-1] = '\0';

	strncpy(variables[index_pub].name, cmd, len_name); /* copy the left part of the string (name) in
							  variable[index]->name */

	variables[index_pub].name[len_name] = '\0'; /* add terminate character at the end of the string name */
	
	index_pub++;
}


/* void function that prints info for our shell */

void print_prompt(){
	char * user=getlogin();
	char path[PATH_MAX];
	getcwd(path,sizeof(path));

	printf("csd5193-hy345sh@%s:%s$ ", user, path);
	return;
}

/* checking if cmd is a valid command in unix 
 * returns 1 -> command exists
 * returns 0 -> command does not exist
 * */

int is_unix_cmd(const char *cmd) {
    	char command[256];
    	sprintf(command, "command -v %s > /dev/null 2>&1", cmd);
    	int result = system(command); 

    	if (result == 0) {
        	return 1; 
    	} else {
        	return 0;  
    	}
}

/*
 *	pid=-1 then it means that the new process was not created
 *	pid= 0 means we are in the child process
 *	pid> 0 means we are in the parent process
 */

void execute_cmd(char **tokens){
	int i=0;
	do{
		if(tokens[i]==NULL){
			printf("Empty command\n");
        		return;
		}
		
		if(is_unix_cmd(tokens[i])==1){
		//if(strcmp(tokens[i],"ls")==0 || strcmp(tokens[i],"date")==0 || strcmp(tokens[i],"cat")==0){
			pid_t pid=fork();

			if(pid==-1){
				perror("fail fork :(");
				return;

			}else if(pid==0){
				
				if(execvp(tokens[i],tokens)==-1)
					perror("execvp error");
				
				_exit(1);

			}else{

				wait(NULL);
			}
		}

		i++;
	}while(tokens[i]!=NULL);
}

/* here we broke the cmd based in ; delimiter and we call execute for each command */

char ** split_cmd_semi(char* cmd){
	
	int position=0;
	char **tokens=malloc(sizeof(char*) * CMD_SIZE);
	char *token=malloc(CMD_SIZE*sizeof(char));


    	token=strtok(cmd,";");
	while(token!=NULL){
		tokens[position]=token;

		token=strtok(NULL,";");
		position++;
	}
	tokens[position]=NULL;

	for (int i = 0; i < position; i++) {
        
        	char *args[CMD_SIZE];
        	int j = 0;
        	token = strtok(tokens[i], LINE_DEL);
        	while (token != NULL) {
            		args[j++] = token;
            		token = strtok(NULL, LINE_DEL);
        	}
        	args[j] = NULL; 

        	execute_cmd(args);
    	}

	return tokens;
}

/* I break the string into tokens based on the delimiters I have defined */

char ** split_cmd(char *cmd){
	int position=0;
	char ** tokens=malloc(sizeof(char *) * CMD_SIZE);
	char * token=malloc(CMD_SIZE * sizeof(char));

	if(!tokens){
        	fprintf(stderr, "hy345sh: Allocation Error\n");
        	exit(EXIT_FAILURE);
    	}

	
	token = strtok(cmd, LINE_DEL);
	while(token!=NULL){
		tokens[position]=token;
		
		token=strtok(NULL,LINE_DEL);

		position++;
	}
	
	tokens[position]=NULL;
	return tokens;
}

/* removes the white spaces from the buffer */

void remove_space(char *buf){
	if(buf[strlen(buf)-1]==' ' || buf[strlen(buf)-1]=='\n')
		buf[strlen(buf)-1]='\0';
	if(buf[0]==' ' || buf[0]=='\n') 
		memmove(buf, buf+1, strlen(buf));
}

/* in shell function i handle various cases */

void shell(){
	int input=1;
	int output=0;
	int append=-1;
	
	char* cmd=NULL;
	cmd=malloc(sizeof(CMD_SIZE));
	if(cmd==NULL) return;

	int status=1;int i=0;
	char **tokens;

	do{
		/* display terminal details */
		
		print_prompt();
		
		/* takes the input from the terminal and stores it in the cmd string */

		fgets(cmd, CMD_SIZE, stdin);
		
		/* multi pipes */

		if (strchr(cmd,'|') != NULL){
	
			char * buffer[128];
			char * token=strtok(cmd,"|");
			int cmds_num=0;

			while(token!=NULL){
				
                		buffer[cmds_num]=malloc(sizeof(token)+1);
				strcpy(buffer[cmds_num],token);

				remove_space(buffer[cmds_num]);

				cmds_num++;
				token=strtok(NULL,"|");
			}
			
			buffer[cmds_num]=NULL;

			execute_pipes(buffer,cmds_num);
			
			continue;
		}

		/* file1 becomes the input for command1 */

		if (strchr(cmd,'<') != NULL){
			
			char * buffer[ARR_SIZE];
			char * token=strtok(cmd,"<");
			int cmds_num=0;
			
			while(token!=NULL){
				
				buffer[cmds_num]=malloc(sizeof(token)+1);
				strcpy(buffer[cmds_num],token);

				remove_space(buffer[cmds_num]);
				
				cmds_num++;
				token=strtok(NULL,"<");
			}	
		
			buffer[cmds_num]=NULL;

			if(cmds_num==2)
				redirect(buffer,input);
			else
				printf("Error: no valid form . Try command1 < file1\n");

			continue;
		}

		/* the output of command2 stores to file2 without delete his written data */	
		
		if (strstr(cmd,">>") != NULL){

			char *buffer[ARR_SIZE];
			char * token =strtok(cmd,">>");
			int cmds_num=0;

			while(token != NULL){
				
				buffer[cmds_num]=malloc(sizeof(token)+1);
				strcpy(buffer[cmds_num],token);

				remove_space(buffer[cmds_num]);

				cmds_num++;
				token=strtok(NULL,">>");
			}

			buffer[cmds_num]=NULL;

			if(cmds_num==2)
				redirect(buffer,append);
			else
				printf("Error: no valid form . Try command2 >> file2\n");

			continue;
		}

		/* the output of command3 stores to file3 */

		if (strchr(cmd,'>') != NULL){
			
			char *buffer[ARR_SIZE];
			char * token=strtok(cmd,">");
			int cmds_num=0;

			while(token!=NULL){

				buffer[cmds_num]=malloc(sizeof(token)+1);
				strcpy(buffer[cmds_num],token);

				remove_space(buffer[cmds_num]);

				cmds_num++;
				token=strtok(NULL,">");
			}
			
			buffer[cmds_num]=NULL;

			if(cmds_num==2)
				redirect(buffer,output);
			else
				printf("Error: no valid form . Try command3 > file3\n");

			continue;
		}

		/* it checks if it has the substring exit in the cmd string and 
		 * if it has the status 0 and terminates */

		if (strstr(cmd, "exit") != NULL) {
            		printf("Exiting shell...\n");
           		status=0;
			continue;
        	}
		
		/* check if cmd has ; */

		if (strchr(cmd, ';') != NULL) {
			tokens=split_cmd_semi(cmd);
			/*	
			i=0;
			while(tokens[i]!=NULL){
				printf("%s\n",tokens[i]);
				i++;
			}*/
			continue;	
		}
			
		/* checks if cmd has = */

		if(strchr(cmd,'=')!=NULL){

			createVar(cmd);
			/*	
			for (int i = 0; i < index_pub; i++) {
        			printf("Name: %s, Value: %s\n", variables[i].name, variables[i].value);
			}*/
				
			continue;
		}

		/* checks if echo */

		if(strstr(cmd,"echo")!=NULL){

			handleEcho(cmd);

			continue;
		}

		tokens=	split_cmd(cmd);
		/*	
		i=0;
		while(tokens[i]!=NULL){
			printf("%s\n",tokens[i]);
			i++;	
		}*/

		execute_cmd(tokens);

	}while(status);
}

/* here I am called shell */

int main(){

	shell();

	return 0;
}
