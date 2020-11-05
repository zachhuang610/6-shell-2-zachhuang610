# 6-shell-2

Shell 2 expands on Shell 1 in the following ways:
1.) Multiple Jobs
    allows user inputs with & to be parsed, exec_program changes
    terminal control and reaps if it is a foreground task. Otherwise
    adds to job list.
2.) Reaping
    Reaping occurs in the following places: fg, exec_program, main
        fg: WUNTRACED option, waits until fg process terminates, printing correct statement
        exec_program: WUNTRACED option, waits until fg process terminates
        if it's foreground, prints statement corresponding to termination
        state. 
        main: WNOHANG | WCONTINUED options, waits until process state changes
        prints correct correct status. iterates through all processes in jobs

3.) fg/bg
    fg: given a jid, jlist, gets pgid, sets terminal control to pgid, sends
    SIGCONT signal to pig process. waits on process to terminate, producing
    correct result and updating jlist accordingly.
    bg: given a jid, jlist, gets pgid, sends SIGCONT, and updates jlist.
4.) Signal handling
    In shell, ignores SIGTTOU, SIGINT, SIGTSTP, SIGQUIT. in any child process
    resets to SIGDFL using signal().

In addition, main was modified to include jobs, fg, and bg, which each call the
corresponding helper function.