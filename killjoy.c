/**
 * Copyright 2013, Rob Lyon <nosignsoflifehere@gmail.com>
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

// Keeping this here for now so I can figure out what I was thinking when
// I was redefining WIFSIGNALED.  I think it was probably due to wanting
// to handle SIGINT a bit differently, but it's been long enough that I
// don't actually remember.
// #undef WIFSIGNALED
// #define WIFSIGNALED(x) (!WIFEXITED(x) && !WIFSTOPPED(x))

static int version[3] = {1, 0, 0};
static int opt_quiet;
static int opt_seconds;
static int opt_debug;

void help(void) {
    printf("killjoy version %d.%d.%d\n", version[0], version[1], version[2]);
    printf("\n");
    printf("Usage: killjoy [options] [utility]\n");
    printf("\n");
    printf("Options\n");
    printf(" -t, --time=[D:[H:[M:[S]]]]\tdays, hours, minutes, and seconds to run the utility\n");
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

int timer(pid_t pid) {
    pid_t w;
    int status;
    int start = (int) time(NULL);
    int running;

    debug("pid: %d", pid);

    do {
        sleep (1);
        w = waitpid (pid, &status, WNOHANG);
        if (w < 0) {
            info ("Process has completed or is no longer running");
            return status;
        }

        running = (int) time(NULL) - start;

        debug("Checking: %d running <= %d opt_seconds", running, opt_seconds);
        debug("WIFSIGNALED: %d", WIFSIGNALED(status));
    } while (!WIFSIGNALED(status) && running <= opt_seconds);

    if (running > opt_seconds) {
        info ("Process has exceeded the time limit of %d seconds", opt_seconds);
        kill (pid,SIGKILL);
        return SIGKILL;
    }
    
    return status;
}

void run_command(char *const *cmd) {
    pid_t pid;
    pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        execvp (cmd[0], cmd);
        perror ("execute");
        exit(errno == ENOENT ? 127 : 126);
    } else {
        int status = timer(pid);
        if (WIFSTOPPED(status)) {
            info ("Process was stopped with signal: %d", WSTOPSIG(status));
        } else if (WIFSIGNALED(status)) {
            info ("Process was terminated with signal: %d", WTERMSIG(status));
        } else if (WIFEXITED(status)) {
            info ("Process has exited with status: %d", WEXITSTATUS(status));
        }
    }
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
        { 0, 0, 0, 0}
    };

    int c;
    int option_index;
    opt_quiet = 0;
    opt_seconds = 0;

    while( 1 ) {
        c = getopt_long( argc, argv, "t:qd", long_options, &option_index );
        if( c == -1 ) break;
        switch( c ) {
        case 't':
            opt_seconds = parse_timevalue(optarg);
            break;
        case 'd':
            opt_debug = 1;
            break;
        case 'q':
            opt_quiet = 1;
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
