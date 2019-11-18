/**
 * Copyright 2013, Rob Lyon <rob@ctxswitch.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#undef WIFSIGNALED
#define WIFSIGNALED(x) (!WIFEXITED(x) && !WIFSTOPPED(x))

pid_t pid = (pid_t)-1;
int start;

static int version[3] = {1, 1, 0};
static int opt_quiet;
static int opt_seconds;
static int opt_signal = SIGKILL;
static int opt_debug = 0;

struct signal {
    char* name;
    int value;
    int handled;
};

struct signal signals[] = {
    {"hup",     SIGHUP,     0},
    {"int",     SIGINT,     1},
    {"quit",    SIGQUIT,    1},
    {"ill",     SIGILL,     1},
    {"trap",    SIGTRAP,    1},
    {"abrt",    SIGABRT,    1},
    {"iot",     SIGIOT,     1},
    {"bus",     SIGBUS,     1},
    {"fpe",     SIGFPE,     1},
    {"kill",    SIGKILL,    0},
    {"usr1",    SIGUSR1,    1},
    {"segv",    SIGSEGV,    1},
    {"usr2",    SIGUSR2,    1},
    {"pipe",    SIGPIPE,    1},
    {"alrm",    SIGALRM,    1},
    {"term",    SIGTERM,    1},
    {"stkflt",  SIGSTKFLT,  1},
    {"chld",    SIGCHLD,    1},
    {"cont",    SIGCONT,    0},
    {"stop",    SIGSTOP,    0},
    {"tstp",    SIGTSTP,    1},
    {"ttin",    SIGTTIN,    1},
    {"ttou",    SIGTTOU,    1},
    {"urg",     SIGURG,     1},
    {"xcpu",    SIGXCPU,    1},
    {"xfsz",    SIGXFSZ,    1},
    {"vtalrm",  SIGVTALRM,  1},
    {"prof",    SIGPROF,    1},
    {"winch",   SIGWINCH,   1},
    {"io",      SIGIO,      1},
    {"poll",    SIGPOLL,    1},
    {0, 0, 0}
};

void help(void) {
    printf("killjoy version %d.%d.%d\n", version[0], version[1], version[2]);
    printf("\n");
    printf("Usage: killjoy [options] [utility]\n");
    printf("\n");
    printf("Options\n");
    printf(" -t, --time=[D:[H:[M:[S]]]]\tdays, hours, minutes, and seconds to run the utility\n");
    printf(" -s, --signal=[SIGNAL]\tthe signal used to terminate the process.  Default: SIGKILL\n");
    printf(" -q, --quiet\t\t\tdon't output any informative messages\n");
    printf(" -h, --help\t\t\tdisplay this help message\n");
    printf("\n");
    exit(1);
}

void info(const char *format, ...) {
    if (!opt_quiet) {
        va_list argp;
        fprintf(stderr, "KILLJOY [info]: ");
        va_start(argp, format);
        vfprintf(stderr, format, argp);
        va_end(argp);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

void error(const char *format, ...) {
    va_list argp;
    fprintf(stderr, "KILLJOY [error]: ");
    va_start(argp, format);
    vfprintf(stderr, format, argp);
    va_end(argp);
    fprintf(stderr, "\n");
    fflush(stderr);
}

void debug(const char *format, ...) {
    if (opt_debug) {
        va_list argp;
        fprintf(stderr, "KILLJOY [debug]: ");
        va_start(argp, format);
        vfprintf(stderr, format, argp);
        va_end(argp);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

int timer() {
    pid_t w;
    int status;
    int running;

    start = (int) time(NULL);

    debug("Child pid: %d", pid);
    debug("Start time: %d", start);

    do {
        sleep (1);
        w = waitpid (pid, &status, WNOHANG);
        if (w < 0) {
            info ("Process has completed or is no longer running");
            return status;
        }

        running = (int) time(NULL) - start;

        debug("Checking: %d running <= %d opt_seconds", running, opt_seconds);
        // debug("WIFEXITED: %x", WIFEXITED(status));
        // debug("WIFSTOPPED: %x", WIFSTOPPED(status));
        // debug("WIFSIGNALED: %x", WIFSIGNALED(status));
    } while (WIFSIGNALED(status) && running <= opt_seconds);

    if (running > opt_seconds) {
        info ("Process has exceeded the time limit of %d seconds", opt_seconds);
        kill (pid, opt_signal);
        return opt_signal;
    }

    return status;
}

void signal_handler(int signo) {
    if (signo == SIGUSR2) {
        start = (int) time(NULL);
    } else if (signo == SIGHUP) {
        // Thinking that I would like to somthing seperately in the future with this.
        debug("Signal %d has been caught.  Ignoring", signo);
        kill(pid, signo);
    } else {
        debug("Signal %d has been caught.  Passing it to the child before exiting.", signo);
        kill(pid, signo);
        exit(0);
    }
}

void run_command(char *const *cmd) {
    pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        execvp (cmd[0], cmd);
        perror ("execute");
        exit(errno == ENOENT ? 127 : 126);
    } else {
        for (int i = 0; signals[i].value; i++) {
            if (signals[i].handled == 1 && signal(signals[i].value, signal_handler) == SIG_ERR) {
                error("Unable to set up signal handling for %d", signals[i].value);
                kill(pid, SIGKILL);
            }
        }

        int status = timer();
        if (WIFSTOPPED(status)) {
            info ("Process was stopped with signal: %d", WSTOPSIG(status));
        } else if (WIFSIGNALED(status)) {
            info ("Process was terminated with signal: %d", WTERMSIG(status));
        } else if (WIFEXITED(status)) {
            info ("Process has exited with status: %d", WEXITSTATUS(status));
        }
    }
}

int parse_signal(char* signal) {
    // Lowercase the string for the mapping
    char* s = signal;
    for(; *s; ++s) *s = tolower(*s);

    for(int i = 0; signals[i].name; i++) {
        if (strcmp(signals[i].name, signal) == 0) {
            debug("Found mapping for signal: %s (%d)", signals[i].name, signals[i].value);
            return signals[i].value;
        } else if (atoi(signal) == signals[i].value) {
            return signals[i].value;
        }
    }

    return 0;
}

int parse_timevalue(char* timestr) {
    // Days, Hours, Minutes, Seconds
    int times[4] = { 0,0,0,0 };
    
    int i = 0;
    char *start = timestr;
    char *end = timestr + strlen(timestr);
    
    // Start at the end and work back.
    while( end != start ) {
        // Quietly bail
        if (i > 3) break;
        // Catch the seperator
        if (*end == ':') {
            times[i++] = atoi(end + 1);
            *end = '\0';
        }
        end--;
    }
    times[i] = atoi(end);

    return times[0] + (times[1] * 60) + (times[2] * 3600) + (times[3] * 86400);
}

int get_options(int argc, char *argv[]) {
    // Options.  Time should be in the H:M format. 
    struct option long_options[] = {
        { "quiet",  no_argument,        0,          'q' },
        { "help",   0,                  0,          'h' },
        { "debug",  no_argument,        0,          'd' },
        { "time",   required_argument,  0,          't' },
        { "signal", required_argument,  0,          's' },
        { 0, 0, 0, 0}
    };

    int c;
    int option_index;
    opt_quiet = 0;
    opt_seconds = 0;

    while( 1 ) {
        c = getopt_long( argc, argv, "t:s:qd", long_options, &option_index );
        if ( c == -1 ) break;
        switch( c ) {
        case 't':
            opt_seconds = parse_timevalue(optarg);
            if (opt_seconds < 1) {
                error("Please set the time equal or longer than 1 second.");
                exit(1);
            }
            break;
        case 'd':
            opt_debug = 1;
            break;
        case 'q':
            opt_quiet = 1;
            break;
        case 's':
            opt_signal = parse_signal(optarg);
            if (opt_signal < 1) {
                error("Unsupported signal: %s (%d)\n", optarg, opt_signal);
                exit(1);
            }
            break;
        case 'h':
        default:
            help();
            break;
        }
    }
    return optind;
}

int main (int argc, char *argv[]) {
    int options, i;
    
    options = get_options(argc, argv);
    if (options < argc) {
        argv += options;
        argc -= options;
        info ("Sucking all the life from '%s' in %d seconds", argv[0], opt_seconds);
        run_command(argv);
    }
}
