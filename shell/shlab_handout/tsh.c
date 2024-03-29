/* 
 * tsh - A tiny shell program with job control
 * 
 * <Pranav Rajan u1136324>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* 
 * csapp.c - Functions for the CS:APP3e book
 *
 * Updated 10/2016 reb:
 *   - Fixed bug in sio_ltoa that didn't cover negative numbers
 *
 * Updated 2/2016 droh:
 *   - Updated open_clientfd and open_listenfd to fail more gracefully
 *
 * Updated 8/2014 droh: 
 *   - New versions of open_clientfd and open_listenfd are reentrant and
 *     protocol independent.
 *
 *   - Added protocol-independent inet_ntop and inet_pton functions. The
 *     inet_ntoa and inet_aton functions are obsolete.
 *
 * Updated 7/2014 droh:
 *   - Aded reentrant sio (signal-safe I/O) routines
 * 
 * Updated 4/2013 droh: 
 *   - rio_readlineb: fixed edge case bug
 *   - rio_readnb: removed redundant EINTR check
 */

ssize_t sio_puts(char s[]);
ssize_t sio_putl(long v);
static size_t sio_strlen(char s[]);
static void sio_ltoa(long v, char s[], int b);
static void sio_reverse(char s[]);

/* Misc manifest constants */
#define MAXLINE 1024   /* max line size */
#define MAXARGS 128    /* max args on a command line */
#define MAXJOBS 16     /* max jobs at any point in time */
#define MAXJID 1 << 16 /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;   /* defined in libc */
char prompt[] = "tsh> "; /* command line prompt (DO NOT CHANGE) */
int verbose = 0;         /* if true, print additional output */
int nextjid = 1;         /* next job ID to allocate */
char sbuf[MAXLINE];      /* for composing sprintf messages */
volatile int fg_pid;     // variable for keeping track of the pid of the fg process

struct job_t
{                        /* The job struct */
  pid_t pid;             /* job PID */
  int jid;               /* job ID [1, 2, ...] */
  int state;             /* UNDEF, BG, FG, or ST */
  char cmdline[MAXLINE]; /* command line */
};

struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */

/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bg(int jid);
void do_fg(int jid);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv, int cmdnum);
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs);
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid);
int pid2jid(pid_t pid);
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv)
{
  char c;
  char cmdline[MAXLINE];
  int emit_prompt = 1; /* emit prompt (default) */

  /* Redirect stderr to stdout (so that driver will get all output
   * on the pipe connected to stdout) */
  dup2(1, 2);

  /* Parse the command line */
  while ((c = getopt(argc, argv, "hvp")) != EOF)
  {
    switch (c)
    {
    case 'h': /* print help message */
      usage();
      break;
    case 'v': /* emit additional diagnostic info */
      verbose = 1;
      break;
    case 'p':          /* don't print a prompt */
      emit_prompt = 0; /* handy for automatic testing */
      break;
    default:
      usage();
    }
  }

  /* Install the signal handlers */

  /* These are the ones you will need to implement */
  Signal(SIGINT, sigint_handler);   /* ctrl-c */
  Signal(SIGTSTP, sigtstp_handler); /* ctrl-z */
  Signal(SIGCHLD, sigchld_handler); /* Terminated or stopped child */

  /* This one provides a clean way to kill the shell */
  Signal(SIGQUIT, sigquit_handler);

  /* Initialize the job list */
  initjobs(jobs);

  /* Execute the shell's read/eval loop */
  while (1)
  {

    /* Read command line */
    if (emit_prompt)
    {
      printf("%s", prompt);
      fflush(stdout);
    }
    if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
      app_error("fgets error");
    if (feof(stdin))
    { /* End of file (ctrl-d) */
      fflush(stdout);
      exit(0);
    }

    /* Evaluate the command line */
    eval(cmdline);
    fflush(stdout);
    fflush(stdout);
  }

  exit(0); /* control never reaches here */
}

/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
 */
void eval(char *cmdline)
{
  char *argv1[MAXARGS]; /* argv for execve() */
  char *argv2[MAXARGS]; /* argv for second command execve() */
  int bg;               /* should the job run in bg or fg? */
  pid_t pid;            // pid for forking
  pid_t pid2;           // second pid for forking two commands
  int fds[2];           // array for the piping

  // Set up signals for blocking
  sigset_t mask_all, prev_all; // set up sig sets
  sigfillset(&mask_all);       // add all the signals for blocking for adding a job

  /* If the line contains two commands, split into two strings */
  char *cmd2 = strchr(cmdline, '|');

  if (cmd2 != NULL && strlen(cmd2) >= 3 && (cmd2 - cmdline) >= 2)
  {
    // Terminate the first command with newline and null character
    cmd2--;
    cmd2[0] = '\n';
    cmd2[1] = '\0';
    // Set the second command to start after the next space
    cmd2 += 3;
  }

  /* Parse command line */
  bg = parseline(cmdline, argv1, 1);
  if (argv1[0] == NULL)
    return; /* ignore empty lines */

  if (cmd2 != NULL)
    parseline(cmd2, argv2, 2);

  // TODO: Execute the command(s)
  //       If cmd2 is NULL, then there is only one command

  if (!builtin_cmd(argv1))
  {
    // child runs the job
    // this section is from textbook page 755, 765
    // block all signals and save previous blocked set
    sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
    if (cmd2 != NULL)
    {
      // set up the pipe for the file descriptors
      pipe(fds);
    }

    if ((pid = fork()) == 0)
    {
      setpgid(0, 0);

      if (cmd2 != NULL)
      {
        // redirect the file descriptor write end to a new write end
        dup2(fds[1], 1);
        // close the ends of the pipe for the parent process according to Flatt pipe videos
        close(fds[0]);
        close(fds[1]);
      }

      // unblock SIGCHLD and other signals before execve
      sigprocmask(SIG_SETMASK, &prev_all, NULL);
      if (execve(argv1[0], argv1, environ) < 0)
      {
        printf("%s: Command not found\n", argv1[0]);
        exit(0);
      }
    }

    if (cmd2 != NULL)
    {
      // // child runs the job
      // // this section is from textbook page 755, 765
      if ((pid2 = fork()) == 0)
      {
        setpgid(0, 0);

        // send the information from the write end of the pipe to the read end for the second process
        dup2(fds[0], 0);
        // close the ends of the pipe for the parent process according to Flatt pipe videos
        close(fds[0]);
        close(fds[1]);
        // unblock SIGCHLD and other signals before execve
        sigprocmask(SIG_SETMASK, &prev_all, NULL);

        // execute the second command that we got from the pipe
        if (execve(argv2[0], argv2, environ) < 0)
        {
          printf("%s: Command not found\n", argv2[0]);
          exit(0);
        }
      }
    }

    // close the ends of the pipe for the parent process according to Flatt pipe videos
    if (cmd2 != NULL)
    {
      close(fds[0]);
      close(fds[1]);
    }

    // wait for foreground process to terminate
    if (!bg)
    {
      // add job
      // // block all signals while waiting for adding a job page 777
      if (cmd2 != NULL)
      {
        // piped command is always a foreground process
        addjob(jobs, pid2, FG, cmdline);
        // add a background job for the first command
        addjob(jobs, pid, BG, cmdline);
      }
      else
      {
        // case when we only have one command
        addjob(jobs, pid, FG, cmdline);
      }
      // unblock all signals after adding a job
      sigprocmask(SIG_SETMASK, &prev_all, NULL);

      // wait for the foreground process to finish
      if (cmd2 != NULL)
      {
        fg_pid = pid2;
        waitfg(pid2);
      }
      else
      {
        fg_pid = pid;
        waitfg(pid);
      }
    }
    else
    {
      addjob(jobs, pid, BG, cmdline);
      // unblock all signals after adding a job
      sigprocmask(SIG_SETMASK, &prev_all, NULL);
      printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
    }
  }

  return;
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv, int cmdnum)
{
  static char array1[MAXLINE]; /* holds local copy of command line */
  static char array2[MAXLINE]; /* holds local copy of 2nd command line */
  char *buf;                   /* ptr that traverses command line */
  char *delim;                 /* points to first space delimiter */
  int argc;                    /* number of args */
  int bg;                      /* background job? */

  if (cmdnum == 1)
    buf = array1;
  else
    buf = array2;

  strcpy(buf, cmdline);

  buf[strlen(buf) - 1] = ' ';   /* replace trailing '\n' with space */
  while (*buf && (*buf == ' ')) /* ignore leading spaces */
    buf++;

  /* Build the argv list */
  argc = 0;
  if (*buf == '\'')
  {
    buf++;
    delim = strchr(buf, '\'');
  }
  else
  {
    delim = strchr(buf, ' ');
  }

  while (delim)
  {
    argv[argc++] = buf;
    *delim = '\0';
    buf = delim + 1;
    while (*buf && (*buf == ' ')) /* ignore spaces */
      buf++;

    if (*buf == '\'')
    {
      buf++;
      delim = strchr(buf, '\'');
    }
    else
    {
      delim = strchr(buf, ' ');
    }
  }
  argv[argc] = NULL;

  if (argc == 0) /* ignore blank line */
    return 1;

  /* should the job run in the background? */
  if ((bg = (*argv[argc - 1] == '&')) != 0)
  {
    argv[--argc] = NULL;
  }

  return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv)
{
  char *cmd = argv[0];

  // TODO: Implement "quit" and "jobs" commands
  //       "bg" and "fg" commands are partially handled here,
  //       but you need to implement the do_bg and do_fg functions.

  // quit
  if (!strcmp(cmd, "quit"))
  {
    exit(0);
  }

  // jobs
  if (!strcmp(cmd, "jobs"))
  {
    listjobs(jobs);
    return 1;
  }

  if (!strcmp(cmd, "bg") || !strcmp(cmd, "fg"))
  { /* bg and fg commands */

    int jid;

    /* Ignore command if no argument */
    if (argv[1] == NULL)
    {
      printf("%s command requires a %%jobid argument\n", argv[0]);
      return 1;
    }

    if (argv[1][0] == '%')
    {
      jid = atoi(&argv[1][1]);
    }
    else
    {
      printf("%s: argument must be a %%jobid\n", argv[0]);
      return 1;
    }

    if (!strcmp(cmd, "bg"))
      do_bg(jid);
    else
      do_fg(jid);
    return 1;
  }

  if (!strcmp(cmd, "&"))
  { /* Ignore singleton & */
    return 1;
  }

  return 0; /* not a builtin command */
}

/* 
 * do_bg - Execute the builtin bg command
 */
void do_bg(int jid)
{
  // boolean for indicating whether we found a background process
  // non-zero true, zero, false
  int found_bg = 0;
  int i;
  for (i = 0; i < MAXJOBS; i++)
  {
    if (jobs[i].jid == jid)
    {
      found_bg = 1;

      // restart the process if it is currently stopped
      kill(-jobs[i].pid, SIGCONT);

      // change the state from stopped to background
      jobs[i].state = BG;
      printf("[%d] (%d) %s", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
    }
  }

  // We do not find a job
  if (!found_bg)
  {
    printf("%%");
    printf("%d: No such job\n", jid);
  }
  return;
}

/* 
 * do_fg - Execute the builtin fg command
 */
void do_fg(int jid)
{
  // boolean for indicating whether we found a foreground process
  // non-zero true, zero, false
  int found_fg = 0;
  int j;
  for (j = 0; j < MAXJOBS; j++)
  {
    if (jobs[j].jid == jid)
    {
      found_fg = 1;

      // set the fg pid to the current fg proces pid
      fg_pid = jobs[j].pid;

      // if the process is stopped send signal that it should continue
      kill(-jobs[j].pid, SIGCONT);

      // update the state to a foregound state
      jobs[j].state = FG;

      // wait for the foreground process to terminate page 754 textbook
      waitfg(fg_pid);
    }
  }

  // We do not find a job
  if (!found_fg)
  {
    printf("%%");
    printf("%d: No such job\n", jid);
  }
  return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
  struct job_t *fg_job = getjobpid(jobs, fg_pid);
  // wait for foreground process to be get updated
  while (fg_job->state == FG)
  {
    sleep(1);
  }
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig)
{
  // these resources were consulted for how to handle sigchld handler
  // page 743-749, 773, 779 from textbook
  int status;
  pid_t pid;
  int olderrno = errno;
  // Set up signals for blocking
  sigset_t mask_all, prev_all; // set up sig sets
  sigfillset(&mask_all);       // add all the signals for blocking for deleting a job

  while ((pid = waitpid(-1, &status, WUNTRACED | WNOHANG)) > 0)
  {
    // FOUR CASES
    // CASE 4: CHILD THAT CAUSED THE RETURN IS STOPPED
    if (WIFSTOPPED(status))
    {
      // check if the signal that caused the stop was a sigstop or sigstp
      if (WSTOPSIG(status) == SIGSTOP || WSTOPSIG(status) == SIGTSTP)
      {
        // get the job associated with the pid that returns
        struct job_t *job = getjobpid(jobs, pid);
        if (job != NULL && job->state != ST)
        {
          sio_puts("Job [");
          sio_putl(pid2jid(pid));
          sio_puts("] ");
          sio_puts("(");
          sio_putl(pid);
          sio_puts(") ");
          sio_puts("stopped by signal ");
          sio_putl(WSTOPSIG(status));
          sio_puts("\n");

          // set the job state to stop
          job->state = ST;

          // send the signal back to the process
          kill(-pid, WSTOPSIG(status));
        }
      }
    }
    // CASE 3: CHILD TERMINATED BECAUSE OF SIGNAL NOT CAUGHT
    else if (WIFSIGNALED(status))
    {
      // check if the signal that caused the stop was a sigint
      if (WTERMSIG(status) == SIGINT)
      {
        // get the job associated with the pid that returns
        struct job_t *job = getjobpid(jobs, pid);
        if (job != NULL)
        {
          sio_puts("Job [");
          sio_putl(pid2jid(pid));
          sio_puts("] ");
          sio_puts("(");
          sio_putl(pid);
          sio_puts(") ");
          sio_puts("terminated by signal ");
          sio_putl(WTERMSIG(status));
          sio_puts("\n");

          // block all signals before deleting a job page 779
          sigprocmask(SIG_BLOCK, &mask_all, NULL);
          deletejob(jobs, pid);
          // unblock all signals after deleting a job page 779
          sigprocmask(SIG_SETMASK, &prev_all, NULL);
        }
      }
    }
    // CASE 2: EXIT NORMALLY
    else if (WIFEXITED(status))
    {
      // block all signals before deleting a job page 779
      sigprocmask(SIG_BLOCK, &mask_all, NULL);
      deletejob(jobs, pid);
      // unblock all signals after deleting a job page 779
      sigprocmask(SIG_SETMASK, &prev_all, NULL);
    }
    // CASE 1: OTHER SIGNALS NOT CAUGHT BY THE ABOVE CASES
    else
    {
      sio_puts("Job [");
      sio_putl(pid2jid(pid));
      sio_puts("] ");
      sio_puts("(");
      sio_putl(pid);
      sio_puts(") ");
      sio_puts("terminated by signal ");
      sio_putl(WTERMSIG(status));
      sio_puts("\n");
    }
  }
  errno = olderrno;
  return;
}
/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig)
{
  // this code was adapted from page 769, 774, 779 of the textbook
  int olderrno = errno;
  // Set up signals for blocking
  sigset_t mask_all, prev_all; // set up sig sets
  sigfillset(&mask_all);       // add all the signals for blocking for deleting a job
  if (fg_pid)
  {
    sio_puts("Job [");
    sio_putl(pid2jid(fg_pid));
    sio_puts("] ");
    sio_puts("(");
    sio_putl(fg_pid);
    sio_puts(") ");
    sio_puts("terminated by signal ");
    sio_putl(sig);
    sio_puts("\n");

    // delete the job from the job list
    // block all signals before deleting a job page 779 textbook
    sigprocmask(SIG_BLOCK, &mask_all, NULL);
    deletejob(jobs, fg_pid);
    // unblock all signals after deleting a job page 779 textbook
    sigprocmask(SIG_SETMASK, &prev_all, NULL);

    // send signal back to the process
    kill(-fg_pid, SIGINT);
  }
  else
  {
    unix_error("Job does not exist");
  }

  errno = olderrno;

  return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending a SIGTSTP.  
 */
void sigtstp_handler(int sig)
{
  // this code was adapted from page 769, 774, of the textbook
  int olderrno = errno;
  if (fg_pid)
  {

    sio_puts("Job [");
    sio_putl(pid2jid(fg_pid));
    sio_puts("] ");
    sio_puts("(");
    sio_putl(fg_pid);
    sio_puts(") ");
    sio_puts("stopped by signal ");
    sio_putl(sig);
    sio_puts("\n");

    // change the state of the foregroup job to suspend according to textbook page 761
    struct job_t *fg_job = getjobpid(jobs, fg_pid);
    fg_job->state = ST;

    // send the signal to the process
    kill(-fg_pid, SIGTSTP);
  }
  else
  {
    unix_error("Job does not exist");
  }

  errno = olderrno;

  return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job)
{
  job->pid = 0;
  job->jid = 0;
  job->state = UNDEF;
  job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs)
{
  int i;

  for (i = 0; i < MAXJOBS; i++)
    clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs)
{
  int i, max = 0;

  for (i = 0; i < MAXJOBS; i++)
    if (jobs[i].jid > max)
      max = jobs[i].jid;
  return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline)
{
  int i;

  if (pid < 1)
    return 0;

  for (i = 0; i < MAXJOBS; i++)
  {
    if (jobs[i].pid == 0)
    {
      jobs[i].pid = pid;
      jobs[i].state = state;
      jobs[i].jid = nextjid++;
      if (nextjid > MAXJOBS)
        nextjid = 1;
      strcpy(jobs[i].cmdline, cmdline);
      if (verbose)
      {
        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
      }
      return 1;
    }
  }
  printf("Tried to create too many jobs\n");
  return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid)
{
  int i;

  if (pid < 1)
    return 0;

  for (i = 0; i < MAXJOBS; i++)
  {
    if (jobs[i].pid == pid)
    {
      clearjob(&jobs[i]);
      nextjid = maxjid(jobs) + 1;
      return 1;
    }
  }
  return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid)
{
  int i;

  if (pid < 1)
    return NULL;
  for (i = 0; i < MAXJOBS; i++)
    if (jobs[i].pid == pid)
      return &jobs[i];
  return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid)
{
  int i;

  if (jid < 1)
    return NULL;
  for (i = 0; i < MAXJOBS; i++)
    if (jobs[i].jid == jid)
      return &jobs[i];
  return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid)
{
  int i;

  if (pid < 1)
    return 0;
  for (i = 0; i < MAXJOBS; i++)
    if (jobs[i].pid == pid)
    {
      return jobs[i].jid;
    }
  return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs)
{
  int i;

  for (i = 0; i < MAXJOBS; i++)
  {
    if (jobs[i].pid != 0)
    {
      printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
      switch (jobs[i].state)
      {
      case BG:
        printf("Running ");
        break;
      case FG:
        printf("Foreground ");
        break;
      case ST:
        printf("Stopped ");
        break;
      default:
        printf("listjobs: Internal error: job[%d].state=%d ",
               i, jobs[i].state);
      }
      printf("%s", jobs[i].cmdline);
    }
  }
}
/******************************
 * end job list helper routines
 ******************************/

/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void)
{
  printf("Usage: shell [-hvp]\n");
  printf("   -h   print this message\n");
  printf("   -v   print additional diagnostic information\n");
  printf("   -p   do not emit a command prompt\n");
  exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
  fprintf(stdout, "%s: %s\n", msg, strerror(errno));
  exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
  fprintf(stdout, "%s\n", msg);
  exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler)
{
  struct sigaction action, old_action;

  action.sa_handler = handler;
  sigemptyset(&action.sa_mask); /* block sigs of type being handled */
  action.sa_flags = SA_RESTART; /* restart syscalls if possible */

  if (sigaction(signum, &action, &old_action) < 0)
    unix_error("Signal error");
  return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig)
{
  sio_puts("Terminating after receipt of SIGQUIT signal\n");
  exit(1);
}

/* Put string */
ssize_t sio_puts(char s[])
{
  return write(STDOUT_FILENO, s, sio_strlen(s));
}

/* Put long */
ssize_t sio_putl(long v)
{
  char s[128];
  sio_ltoa(v, s, 10); /* Based on K&R itoa() */
  return sio_puts(s);
}

/* sio_strlen - Return length of string (from K&R) */
static size_t sio_strlen(char s[])
{
  int i = 0;
  while (s[i] != '\0')
    ++i;
  return i;
}

/* sio_ltoa - Convert long to base b string (from K&R) */
static void sio_ltoa(long v, char s[], int b)
{
  int c, i = 0;
  int neg = v < 0;

  if (neg)
    v = -v;

  do
  {
    s[i++] = ((c = (v % b)) < 10) ? c + '0' : c - 10 + 'a';
  } while ((v /= b) > 0);

  if (neg)
    s[i++] = '-';

  s[i] = '\0';
  sio_reverse(s);
}

/* sio_reverse - Reverse a string (from K&R) */
static void sio_reverse(char s[])
{
  int c, i, j;

  for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
  {
    c = s[i];
    s[i] = s[j];
    s[j] = c;
  }
}
