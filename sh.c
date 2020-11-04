#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "jobs.h"

int jobn = 0;

/*
parse - a function for parsing user input with redirection feature handling
params:
 - buffer: output of read, a array of char which holds user input
 - tokens: a null-terminated array to hold pointers to tokens
 - argv: a array to hold arguments for functions and commands
 - io: a array to keep track of any IO redirection
*/
int parse(char buffer[1024], char *tokens[512], char *argv[512], char *io[3],
          int *background) {
    int input_redir = 0;
    int output_redir = 0;
    char *token;
    int i = 0;
    token = strtok(buffer, " \n\t");
    while (token != NULL) {
        // checks for redirecion. if so, error checks for proper syntax
        if (strcmp(token, "<") == 0) {
            // checks to see if there has already been an input redirection
            if (input_redir == 0) {
                token = strtok(NULL, " \n\t");  // gets input var
                if (token == NULL) {
                    fprintf(stderr, "syntax error: no input file\n");
                    return -1;
                } else if ((strcmp(token, "<") == 0) ||
                           (strcmp(token, ">") == 0) ||
                           (strcmp(token, ">>") == 0)) {
                    // compares to see if next token is a redir, if so outputs
                    // error and returns error code
                    fprintf(
                        stderr,
                        "syntax error: input file is a redirection symbol\n");
                    return -1;
                } else {
                    input_redir = 1;
                    io[0] = token;                  // sets output to token
                    token = strtok(NULL, " \n\t");  // moves to next token
                }

            } else {
                fprintf(stderr, "syntax error: multiple input files\n");
                return -1;
            }
        } else if (strcmp(token, ">") == 0) {
            if (output_redir == 0) {
                token = strtok(NULL, " \n\t");  // gets input file
                if (token == NULL) {
                    fprintf(stderr, "syntax error: no ouput file\n");
                    return -1;
                } else if ((strcmp(token, "<") == 0) ||
                           (strcmp(token, ">") == 0) ||
                           (strcmp(token, ">>") == 0)) {
                    // compares to see if next token is a redir, if so outputs
                    // error and returns error code
                    fprintf(
                        stderr,
                        "syntax error: output file is a redirection symbol\n");
                    return -1;
                } else {
                    // not an error, next token is a file-looking thing, so
                    // changes correct io stream and continues
                    output_redir = 1;
                    io[1] = token;                  // sets output to token
                    token = strtok(NULL, " \n\t");  // moves to next token
                }
            } else {
                fprintf(stderr, "syntax error: multiple output files\n");
                return -1;
            }
        } else if (strcmp(token, ">>") == 0) {
            if (output_redir == 0) {
                token = strtok(NULL, " \n\t");  // gets input file
                if (token == NULL) {
                    fprintf(stderr, "syntax error: no ouput file \n");
                    return -1;
                } else if ((strcmp(token, "<") == 0) ||
                           (strcmp(token, ">") == 0) ||
                           (strcmp(token, ">>") == 0)) {
                    // compares to see if next token is a redir, if so outputs
                    // error and returns error code
                    fprintf(
                        stderr,
                        "syntax error: output file is a redirection symbol\n");
                    return -1;
                } else {
                    // not an error, next token is a file-looking thing, so
                    // changes correct io stream and continues
                    output_redir = 1;
                    io[2] = token;                  // sets output to token
                    token = strtok(NULL, " \n\t");  // moves to next token
                }
            } else {
                fprintf(stderr, "syntax error: multiple output files\n");
                return -1;
            }
        } else {
            // not a redirection symbol
            tokens[i] = token;              // adds token to token array
            token = strtok(NULL, " \n\t");  // moves to next token
            if ((token == NULL) && (strcmp(tokens[i], "&") == 0)) {
                *background = 1;
                tokens[i] = token;
            } else {
                i++;
            }
        }
    }

    for (int j = 0; j < i; j++) {
        char *arg;
        arg = strrchr(tokens[j], '/');
        if (arg == NULL) {
            argv[j] = tokens[j];
        } else {
            argv[j] = arg + 1;
        }
    }
    argv[i] = NULL;
    return 0;
}

/*
exec_program: a function for forking and executing custom commands and or
programs params:
- filename: the command or filename to execute
- argv: a null-terminated array of arguments to filename
- io: an array detailing any IO redirection to do
output: NONE
*/
void exec_program(char *filename, char *argv[512], char *io[3], int background,
                  job_list_t *jlist) {
    int pid;
    int wstatus;

    if ((pid = fork()) == 0) {
        setpgid(0, 0);

        if (background == 0) {
            tcsetpgrp(0, getpgrp());
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
        }

        // SET IO REDIRECTION IF EXISTS
        if (io[0] != 0) {
            // checks to see if there is input redirection
            int n = close(0);
            if (n == -1) {
                perror("close");
                exit(1);
            }
            n = open(io[0], O_RDONLY);
            if (n == -1) {
                perror(io[0]);
                exit(1);
            }
        }
        if (io[1] != 0) {
            // checks to see if there is output redirection
            int n = close(1);
            if (n == -1) {
                perror("close");
                exit(1);
            }
            n = open(io[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (n == -1) {
                perror(io[1]);
                exit(1);
            }
        }
        if (io[2] != 0) {
            // checks to see if there is append-output redirection
            int n = close(1);
            if (n == -1) {
                perror("close");
                exit(1);
            }
            n = open(io[2], O_WRONLY | O_CREAT | O_APPEND, 0666);
            if (n == -1) {
                perror(io[2]);
                exit(1);
            }
        }
        execv(filename, argv);
        perror("execv");
        exit(1);
    }

    if (background == 0) {
        waitpid(pid, &wstatus, WUNTRACED);
        if (WIFSIGNALED(wstatus)) {
            // terminated by signal
            fprintf(stdout, "[%i] (%i) terminated by signal %i \n", jobn, pid,
                    WTERMSIG(wstatus));
        } else if (WIFSTOPPED(wstatus)) {
            // stopped, adds to job list
            jobn++;
            add_job(jlist, jobn, pid, STOPPED, filename);
            fprintf(stdout, "[%i] (%i) suspended by signal %i \n", jobn, pid,
                    WSTOPSIG(wstatus));
        }
    } else {
        jobn++;
        add_job(jlist, jobn, pid, RUNNING, filename);
        fprintf(stdout, "[%i] (%i) \n", jobn, pid);
        fflush(stdout);
    }
    tcsetpgrp(0, getpgrp());
}

/*
main: the main shell, which waits for user input, parses and execute user
input until an EOF has been detected
*/
int main() {
    ssize_t n;
    int BUFFER_SIZE = 1024;
    char buf[BUFFER_SIZE];
    char *tokens[512];
    char *argv[512];
    char *io[3];

    job_list_t *jlist = init_job_list();
    int ret, status;

    while (1) {
        signal(SIGINT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);

        while (((ret = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0)) {
            if (WIFEXITED(status)) {
                int jid = get_job_jid(jlist, ret);
                int pid = get_job_pid(jlist, jid);
                printf("[%i] (%i) terminated with exit status %i\n", jid, pid,
                       WEXITSTATUS(status));
                remove_job_jid(jlist, jid);
            } else if (WIFSIGNALED(status)) {
                int jid = get_job_jid(jlist, ret);
                int pid = get_job_pid(jlist, jid);
                printf("[%i] (%i) terminated by signal %i\n", jid, pid,
                       WTERMSIG(status));
                remove_job_jid(jlist, jid);
            } else if (WIFSTOPPED(status)) {
                int jid = get_job_jid(jlist, ret);
                int pid = get_job_pid(jlist, jid);
                update_job_jid(jlist, jid, STOPPED);
                printf("[%i] (%i) suspended by signal %i\n", jid, pid,
                       WSTOPSIG(status));
            } else if (WIFCONTINUED(status)) {
                int jid = get_job_jid(jlist, ret);
                int pid = get_job_pid(jlist, jid);
                update_job_jid(jlist, jid, RUNNING);
                printf("[%i] (%i) resumed", jid, pid);
            }
        }

        tcsetpgrp(0, getpgrp());

#ifdef PROMPT
        if (printf("33sh> ") < 0) {
            /* handle a write error */
        }
        if (fflush(stdout) < 0) {
            /* handle error*/
        }
#endif

        // resets buffers and storage arrays
        memset(argv, 0, 512 * (sizeof(char *)));
        memset(buf, '\0', 1024 * (sizeof(char)));
        memset(tokens, 0, 512 * (sizeof(char *)));
        memset(io, 0, 3 * (sizeof(char *)));
        int background = 0;

        tcsetpgrp(0, getpgrp());
        // reads in input
        n = read(0, buf, sizeof(buf));

        if (n != 0) {
            if (n == -1) {
                fprintf(stderr, "Error reading input");
            } else {
                // todo: parse
                int k = parse(buf, tokens, argv, io, &background);
                // executes below if parse doesn't return an error message
                if (k == 0) {
                    // below compares built-in commands as well as case where
                    // user just presses ENTER, in which case shell loops to
                    // top.
                    char *cmd = tokens[0];
                    if (cmd == NULL) {
                    } else if (strcmp(cmd, "exit") == 0) {
                        cleanup_job_list(jlist);
                        exit(0);
                    } else if (strcmp(cmd, "cd") == 0) {
                        if (tokens[1] != NULL) {
                            int j = chdir(tokens[1]);
                            if (j != 0) {
                                perror("cd");
                            }
                        } else {
                            fprintf(stderr, "cd: syntax error\n");
                        }
                    } else if (strcmp(cmd, "ln") == 0) {
                        // TODO: ln function
                        int j = link(argv[1], argv[2]);
                        if (j != 0) {
                            perror("ln");
                        }
                    } else if (strcmp(cmd, "rm") == 0) {
                        // TODO: rm function
                        int j = unlink(argv[1]);
                        if (j != 0) {
                            perror("rm");
                        }
                    } else if (strcmp(cmd, "fg") == 0) {
                        char *fg_arg = strrchr(tokens[1], '%');
                        if (fg_arg == NULL) {
                            fprintf(stderr, "fg: syntax error");
                        } else {
                            fg_arg++;
                            int jid = atoi(fg_arg);
                            int pid = get_job_pid(jlist, jid);
                            kill(-pid, SIGCONT);
                            pid_t pgid = getpgid(pid);
                            tcsetpgrp(0, pgid);
                            printf("jid: %i\n background to foreground pid: %i\n pgid: %i\n", jid, pid, pgid);
                            fflush(stdout);
                            signal(SIGINT, SIG_DFL);
                            signal(SIGTSTP, SIG_DFL);   
                            signal(SIGQUIT, SIG_DFL);
                            signal(SIGTTOU, SIG_DFL);
                            int wstatus;
                            waitpid(pid, &wstatus, WUNTRACED);
                            if (WIFSIGNALED(wstatus)) {
                                // terminated by signal
                                fprintf(stdout,
                                        "[%i] (%i) terminated by signal %i \n",
                                        jobn, pid, WTERMSIG(wstatus));
                            } else if (WIFSTOPPED(status)) {
                                // stopped, adds to job list
                                jobn++;
                                update_job_jid(jlist, jid, STOPPED);
                                fprintf(stdout,
                                        "[%i] (%i) suspended by signal %i \n",
                                        jobn, pid, WSTOPSIG(wstatus));
                            }
                            remove_job_pid(jlist, pid);
                            tcsetpgrp(0, getpgrp());
                        }
                    } else if (strcmp(cmd, "bg") == 0) {
                    } else {
                        // command is not a built-in, call exec_program
                        exec_program(tokens[0], argv, io, background, jlist);
                    }
                }
            }
        } else {
            cleanup_job_list(jlist);
            exit(0);
        }
    }
}
