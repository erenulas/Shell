#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */

    ct = 0;

    /* read what the user enters on the command line */
    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */

/* the signal interrupted the read system call */
/* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
	exit(-1);           /* terminate with error code of -1 */
    }

	fprintf(">>%s<<",inputBuffer);
    for (i=0;i<length;i++){ /* examine every character in the inputBuffer */

        switch (inputBuffer[i]){
	    case ' ':
	    case '\t' :               /* argument separators */
		if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
		    ct++;
		}
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
		start = -1;
		break;

            case '\n':                 /* should be the final char examined */
		if (start != -1){
                    args[ct] = &inputBuffer[start];
		    ct++;
		}
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
		break;

	    default :             /* some other character */
		if (start == -1)
		    start = i;
                if (inputBuffer[i] == '&'){
		    *background  = 1;
                    inputBuffer[i-1] = '\0';
		}
	} /* end of switch */
     }    /* end of for */
     args[ct] = NULL; /* just in case the input line was > 80 */

	for (i = 0; i <= ct; i++)
		fprintf("args %d = %s\n",i,args[i]);
} /* end of setup routine */

int main(void)
{
            char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
            int background; /* equals 1 if a command is followed by '&' */
            char *args[MAX_LINE/2 + 1]; /*command line arguments */
            int childpid;
            int ret;    /*keeps the return value of chdir*/
            int callFromHistory = 0;    /* if its 0 then it means that the command is not executed through '! string' */
            int lengthOfHist=0; /*number of commands which is recorded in the history*/
            int isPipe=0;   /*checks if a pipe exists*/
            int sizeOfHist=10;  /* size of history buffer */
            int fd[2];          /* array for the pipe */
            int isBackground=0; /* checks if there are any background processes currently running */
            char *pipeCmd[MAX_LINE/2 + 1]; /* keeps the command data for the parent and child in pipe */

            /* struct for the linked list used for history buffer */
            typedef struct hist{
                char *command[MAX_LINE/2 + 1]; /* keeps the command data */
                struct hist *next;
            } hist;

            /* keeps the head of the linked list */
            struct hist *head=NULL;

            while (1){
                        background = 0;
                        /*setup() calls exit() when Control-D is entered */

                        /* checks the status of all child processes, and if the result is 1 it means they are still active,
                        so it sets isBackground to 1, otherwise it'll be set to 0 */
                                int status;
                                pid_t result = waitpid (-1, &status, WNOHANG);
                                if(result==0)
                                    isBackground=1;
                                else
                                    isBackground=0;

                        /* if a '! string' type command is executed, then callFromHistory becomes 1, and
                            it's not needed to get the user command input
                        */
                        if(callFromHistory == 0) {
                            fprintf(stderr,"CSE333sh: ");
                            setup(inputBuffer, args, &background);
                        }

                        /* sets  callFromHistory to 0*/
                        callFromHistory=0;

                        /* if the command is not one of the " cd, dir, clr, wait, hist, !, exit " commands, then enters here */
                        if(strcmp(args[0],"cd")!=0 && strcmp(args[0],"dir")!=0 && strcmp(args[0],"clr")!=0 &&
                            strcmp(args[0],"wait")!=0 && strcmp(args[0],"hist")!=0 && strcmp(args[0],"!")!=0 && strcmp(args[0],"exit")!=0) {

                            /* creates a child */
                            childpid=fork();

                            /* this part is exected only by the child */
                            if(childpid==0){
                                /* checks if a path entered for the command, and if this is the case then it'll use execl */
                                if(*args[0]=='/'){
                                    /* commands with path supports maximum 10 arguments */
                                    execl(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],NULL);
                                /* if a path is not entered, it uses execvp */
                                } else {
                                    /* goes through the arguments to check if there is a pipe operation */
                                    for(int i=0;args[i]!='\0';i++){
                                        if(args[i][0]=='|'){
                                            /* sets isPipe to 1 in order to indicate that there is a pipe operation */
                                            isPipe=1;
                                            break;
                                        }
                                    }

                                    /* if there no pipe operation then it only executes execvp
                                    with the given command and arguments */
                                    if(isPipe==0){
                                        int i=0;
                                        char *temp[MAX_LINE/2 + 1];
                                        /* copies all the arguments except '&' to temp
                                        to prevent any errors that is caused by passing & as an argument to the command */
                                        for(;args[i]!='\0';i++){
                                            if(args[i][0]!='&'){
                                                temp[i] = strdup(args[i]);
                                            }
                                            else {
                                                temp[i]='\0';
                                                break;
                                            }
                                        }
                                        execvp(temp[0],&temp[0]);
                                    /* if there is a pipe operation, enters here */
                                    } else {
                                    /* isPipe is set to 0 */
                                        isPipe=0;

                                        /* Pipe operation */
                                        /* child is the left part of the pipe operator */
                                        if ((pipe(fd) == -1) || ((childpid = fork()) == -1)) {
                                            perror("Failed to setup pipeline");
                                            return 1;
                                        }
                                        /* copies the command which is on the left side of the operator,
                                        and all of its arguments to pipeCmd array */
                                        int i=0;
                                        for(;args[i][0]!='|';i++){
                                            pipeCmd[i] = strdup(args[i]);
                                        }
                                        pipeCmd[i]=NULL; /* sets the last element of the pipeCmd to NULL to prevent any errors */
                                        if (childpid == 0) {
                                            if (dup2(fd[1], STDOUT_FILENO) == -1)
                                                perror("Failed to redirect stdout of command");
                                            else if ((close(fd[0]) == -1) || (close(fd[1]) == -1))
                                                perror("Failed to close extra pipe descriptors on ls");
                                            else {
                                                /* execvp is used for the child operator */
                                                execvp(pipeCmd[0], &pipeCmd[0]);
                                                perror("Failed to exec command");
                                            }
                                            return 1;
                                        }
                                        /* copies the command which is on the right side of the pipe operation,
                                        and all of its arguments to pipeCmd */
                                        int counter=0;
                                        for(i+=1;args[i]!='\0';i++){
                                            pipeCmd[counter] = strdup(args[i]);
                                            counter++;
                                        }
                                        /* last element of pipeCmd is set to NULL */
                                        pipeCmd[counter]=NULL;

                                        if (dup2(fd[0], STDIN_FILENO) == -1)
                                            perror("Failed to redirect stdin of sort");
                                        else if ((close(fd[0]) == -1) || (close(fd[1]) == -1))
                                            perror("Failed to close extra pipe file descriptors on sort");
                                        else {
                                        /* execvp is executed for the parent */
                                            execvp(pipeCmd[0],&pipeCmd[0]);
                                            perror("Failed to exec sort");
                                        }
                                        return 1;


                                    }
                                }
                            }
                        /* if the command is one of "cd, dir, clr, wait, hist, !, exit", then it enters here, and
                        it executes the command without creating a new process */
                        } else {
                            /* cd command */
                             if(strcmp(args[0],"cd") == 0){
                                /* if a path is given for cd then enters here */
                                if(args[1] != NULL){
                                    /* change the directory, and sets the return value of chdir to ret */
                                    ret=chdir(args[1]);
                                    /* if ret is 0, then directory is changed carefully */
                                    if(ret==0){
                                    /* sets the pdw value with the new directory path */
                                        setenv("PDW",args[1],1);
                                    /* if directory is changed successfully, then ret is -1, and prints an error message */
                                    } else if(ret == -1) {
                                        fprintf(stderr,"couldn't change directory\n");
                                    }
                                /* if a path is not given, then enters here */
                                } else {
                                    /* since a path is not given, it changes directory to home,
                                       if ret is 0,then directory is changed successfully */
                                    ret=chdir(getenv("HOME"));
                                    if(ret == 0){
                                        /* updates pwd value */
                                        setenv("PWD","HOME",1);
                                        /* if directory isn't changed successfully, then it enters here, and prints a message */
                                    } else if(ret == -1) {
                                        fprintf(stderr,"couldn't change directory\n");
                                    }
                                }

                                /* if command is dir, then enters here */
                            } else if(strcmp(args[0],"dir") == 0){
                                        /* get current directory value and puts it into path, then prints it */
                                        char *path[1024];
                                        getcwd( path, 1024 );
                                        fprintf(stderr,path);
                                        fprintf(stderr,"\n");

                                /* if the entered command is clr enters here */
                            } else if(strcmp(args[0],"clr") == 0){
                                    /* uses system function to clear the screen*/
                                    char c[50];
                                    strcpy( c, "clear" );
                                    system(c);
                                /* if the entered command is wait, then it waits until the child process is terminated */
                            } else if(strcmp(args[0],"wait") == 0){
                                while(wait(NULL)>0);
                                /* if the entered command is hist, and there isn't any arguments, then enters here */
                            } else if(strcmp(args[0],"hist") == 0 && args[1]=='\0'){
                                /* assigns the head of the linked list to temp */
                                struct hist *temp=head;

                                int i=0;
                                /* if temp value is not null, and i is smaller than the size of the buffer, loop continues */
                                for(;temp!=NULL && i<sizeOfHist;temp=temp->next,i++){
                                    /* prints the command and its arguments */
                                    fprintf(stderr,"%d -)",i);
                                    for(int j=0;temp->command[j]!='\0';j++){
                                        fprintf(stderr,temp->command[j]);
                                        fprintf(stderr," ");
                                    }
                                    fprintf(stderr,"\n");
                                }
                            /* if hist command is entered with '-set' argument, then it enters here */
                            } else if(strcmp(args[0],"hist") == 0 && strcmp(args[1],"-set")== 0){
                                /* takes the number that is entered after -set, and converts it into integer */
                                sizeOfHist = atoi(args[2]);
                                /* prints the previously entered commands which is within the newly set buffer size */
                                struct hist *temp=head;
                                int i=0;
                                for(;temp!=NULL && i<sizeOfHist;temp=temp->next,i++){
                                    fprintf(stderr,"%d -)",i);
                                    for(int j=0;temp->command[j]!='\0';j++){
                                        fprintf(stderr,temp->command[j]);
                                        fprintf(stderr," ");
                                    }
                                    fprintf(stderr,"\n");
                                }
                            /* if '!' command is entered, enters here */
                            } else if(strcmp(args[0],"!") == 0) {
                                /* if the entered argument is a number, then enters here */
                                if(*args[1] == '0' || *args[1] == '1'|| *args[1] == '2'|| *args[1] == '3'|| *args[1] == '4'
                                || *args[1] == '5'|| *args[1] == '6'|| *args[1] == '7'|| *args[1] == '8'|| *args[1] == '9'){
                                    /* converts the argument into integer */
                                    int t = atoi(args[1]);
                                    /* if there is a corresponding command for the entered number, then enters here */
                                    if(t < lengthOfHist && t < sizeOfHist){
                                        /* head of the list is assigned to temp, and
                                        then it moves to the corresponding element of the list */
                                        struct hist *temp=head;
                                        for(int i=0;i!=t;i++){
                                            temp=temp->next;
                                        }
                                        temp = temp->next;
                                        /* sets callFromHistory to 1 to indicate
                                        that the next command will be given from the history */
                                        callFromHistory=1;
                                        int i=0;
                                        /* copies the corresponding command and its arguments to args */
                                        for(;temp->command[i]!='\0';i++){
                                            args[i] = strdup(temp->command[i]);
                                        }
                                        /* sets the last element */
                                        args[i]='\0';
                                        /* if there isn't a corresponding command, then prints a message */
                                    } else {
                                        fprintf(stderr,"there isn't a command which corresponds to number %d.\n",t);
                                    }
                                    /* if the argument that is entered isn't an integer, then enters here */
                                } else {
                                    struct hist *temp=head;
                                    int i=0;
                                    int found=0; /* checks if the command is found or not */
                                    /* keeps moving through the history buffer until it finds a command
                                    whose first two letters correspond to any command in the history buffer */
                                    for(;temp!=NULL && i<sizeOfHist;temp=temp->next,i++){
                                        if(temp->command[0][0] == args[1][0]){
                                            if(temp->command[0][1]==args[1][1]){
                                                callFromHistory=1;
                                                int j=0;
                                                /* if the command is found, then copies it to args */
                                                for(;temp->command[j]!='\0';j++){
                                                    args[j] = temp->command[j];
                                                }
                                                /* found is set to 1 to indicate that a match is found, and breaks the loop */
                                                    found = 1;
                                                    args[j]='\0';
                                                    break;
                                            }
                                        }

                                    }
                                    /* if it doesn't find a match, then enters here, and prints a message */
                                    if(found==0){
                                        fprintf(stderr,"Command you entered doesn't have a match \n");
                                    }

                                }
                                /* if the entered command is exit, then enters here */
                            } else if(strcmp(args[0],"exit") == 0){
                                /* prints a message, and waits for the child process to be terminated, and exits */
                                if(isBackground==1)
                                fprintf(stderr,"Please terminate any working background process in order to exit\n");
                                while(wait(NULL)>0);
                                    exit(0);
                            }
                        }
                        /* if the command is entered without an & argument, then shell waits until the child is terminated. */
                        if(background==0)
                        {
                            while(wait(NULL)>0);
                        }
                        /* if the command isn't called from history, it enters here */
                        if(callFromHistory==0){
                            /* allocates some space for temp */
                            struct hist *temp = (struct hist*) malloc(sizeof(struct hist));
                            /* copies the command and its arguments, to the newly created element of the list */
                            for(int i=0;args[i]!='\0';i++){
                                temp->command[i] = strdup(args[i]);
                            }
                            /* if it is the first element to be added into the list, then it becomes the head */
							if(head==NULL){
                                head = temp;
                                /* if the new element won't be first element in the list,
                                then it is added to the beginning of the list */
                            } else {
                                temp->next = head;
                                head=temp;
                            }
                            /* keeps the number of commands that is added to the list */
                            lengthOfHist++;
                        }



            }
}
