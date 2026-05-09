#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <syslog.h>

#define MAX_ARGS 100
#define MAX_LINE 1024
#define MAX_JOBS 100
#define HIST_SIZE 20
#define PATH_SIZE 4096

typedef struct {
    pid_t pid;
    char cmd[MAX_LINE];
    time_t start;
    int active;
} Job;

static Job jobs[MAX_JOBS] = {0};

static char history[HIST_SIZE][MAX_LINE] = {{0}};
static int hist_count = 0;
static char hist_file[PATH_SIZE] = {0};

static volatile sig_atomic_t show_history = 0;

void safe_copy(char *dest, const char *src, size_t size) {
    if (size == 0) {
        return;
    }

    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
}

void sigquit_handler(int sig) {
    (void)sig;
    show_history = 1;
}

void make_hist_path(void) {
    char *home = getenv("HOME");

    if (home == NULL) {
        fprintf(stderr, "HOME not set\n");
        exit(1);
    }

    snprintf(hist_file, sizeof(hist_file), "%s/.myshell_history", home);
}

void load_history(void) {
    FILE *f = fopen(hist_file, "r");

    if (f == NULL) {
        return;
    }

    char line[MAX_LINE];

    while (fgets(line, sizeof(line), f) != NULL) {
        line[strcspn(line, "\n")] = '\0';

        if (hist_count < HIST_SIZE) {
            safe_copy(history[hist_count], line, sizeof(history[hist_count]));
            hist_count++;
        } else {
            for (int i = 1; i < HIST_SIZE; i++) {
                safe_copy(history[i - 1], history[i], sizeof(history[i - 1]));
            }

            safe_copy(history[HIST_SIZE - 1], line, sizeof(history[HIST_SIZE - 1]));
        }
    }

    fclose(f);
}

void save_history(void) {
    FILE *f = fopen(hist_file, "w");

    if (f == NULL) {
        perror("history");
        return;
    }

    for (int i = 0; i < hist_count; i++) {
        fprintf(f, "%s\n", history[i]);
    }

    fclose(f);
}

void add_history(char *cmd) {
    if (cmd[0] == '\0') {
        return;
    }

    if (hist_count < HIST_SIZE) {
        safe_copy(history[hist_count], cmd, sizeof(history[hist_count]));
        hist_count++;
    } else {
        for (int i = 1; i < HIST_SIZE; i++) {
            safe_copy(history[i - 1], history[i], sizeof(history[i - 1]));
        }

        safe_copy(history[HIST_SIZE - 1], cmd, sizeof(history[HIST_SIZE - 1]));
    }

    save_history();
}

void print_history(void) {
    printf("\n--- history ---\n");

    for (int i = 0; i < hist_count; i++) {
        printf("%2d: %s\n", i + 1, history[i]);
    }

    printf("---------------\n");
    fflush(stdout);
}

void init_jobs(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        jobs[i].active = 0;
    }
}

void add_job(pid_t pid, char *cmd) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].active) {
            jobs[i].pid = pid;
            jobs[i].start = time(NULL);
            jobs[i].active = 1;
            safe_copy(jobs[i].cmd, cmd, sizeof(jobs[i].cmd));
            return;
        }
    }

    fprintf(stderr, "too many background jobs\n");
}

void check_jobs(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active) {
            int status;
            pid_t result = waitpid(jobs[i].pid, &status, WNOHANG);

            if (result == jobs[i].pid) {
                jobs[i].active = 0;
            }
        }
    }
}

void print_jobs(void) {
    check_jobs();

    printf("--- background jobs ---\n");

    int any = 0;

    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active) {
            any = 1;

            char buf[64];
            struct tm *t = localtime(&jobs[i].start);

            if (t != NULL) {
                strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
            } else {
                safe_copy(buf, "unknown", sizeof(buf));
            }

            printf("PID: %d | started: %s | command: %s\n",
                   jobs[i].pid, buf, jobs[i].cmd);
        }
    }

    if (!any) {
        printf("no background jobs\n");
    }

    printf("-----------------------\n");
}

int split_line(char *line, char **args) {
    int n = 0;
    char *token = strtok(line, " \t\n");

    while (token != NULL && n < MAX_ARGS - 1) {
        args[n] = token;
        n++;
        token = strtok(NULL, " \t\n");
    }

    args[n] = NULL;
    return n;
}

int find_redirect(char **args, int *argc, char **file, int *append) {
    *file = NULL;
    *append = 0;

    for (int i = 0; i < *argc; i++) {
        if (strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0) {
            if (i + 1 >= *argc) {
                fprintf(stderr, "missing file after redirection\n");
                return -1;
            }

            if (strcmp(args[i], ">>") == 0) {
                *append = 1;
            }

            *file = args[i + 1];

            for (int j = i; j + 2 <= *argc; j++) {
                args[j] = args[j + 2];
            }

            *argc -= 2;
            args[*argc] = NULL;

            return 0;
        }
    }

    return 0;
}

int builtin_cd(char **args, int argc) {
    if (strcmp(args[0], "cd") != 0) {
        return 0;
    }

    char *dir;

    if (argc == 1) {
        dir = getenv("HOME");
    } else {
        dir = args[1];
    }

    if (dir == NULL) {
        fprintf(stderr, "cd: HOME not set\n");
    } else if (chdir(dir) != 0) {
        perror("cd");
    }

    return 1;
}

void run_command(char **args, int background, char *cmd, char *outfile, int append) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        signal(SIGQUIT, SIG_DFL);

        if (outfile != NULL) {
            int flags = O_WRONLY | O_CREAT;

            if (append) {
                flags |= O_APPEND;
            } else {
                flags |= O_TRUNC;
            }

            int fd = open(outfile, flags, 0644);

            if (fd < 0) {
                perror("open");
                exit(1);
            }

            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("dup2");
                close(fd);
                exit(1);
            }

            close(fd);
        }

        execvp(args[0], args);

        perror("execvp");
        exit(1);
    }

    if (background) {
        add_job(pid, cmd);
        printf("[background] pid: %d\n", pid);
    } else {
        int status;

        while (waitpid(pid, &status, 0) < 0) {
            if (errno == EINTR) {
                if (show_history) {
                    show_history = 0;
                    print_history();
                }
            } else {
                perror("waitpid");
                break;
            }
        }
    }
}

int main(int argc, char **argv) {
    int use_syslog = 0;
    char *script_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--syslog") == 0) {
            use_syslog = 1;
        } else if (script_file == NULL) {
            script_file = argv[i];
        } else {
            fprintf(stderr, "usage: %s [-l|--syslog] [script_file]\n", argv[0]);
            return 1;
        }
    }

    if (use_syslog) {
        openlog("myshell", LOG_PID, LOG_USER);
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigquit_handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGQUIT, &sa, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    make_hist_path();
    load_history();
    init_jobs();

    FILE *input = stdin;

    if (script_file != NULL) {
        input = fopen(script_file, "r");

        if (input == NULL) {
            perror(script_file);
            if (use_syslog) {
                closelog();
            }
            return 1;
        }
    }

    char *line = NULL;
    size_t size = 0;

    while (1) {
        check_jobs();

        if (show_history) {
            show_history = 0;
            print_history();
        }

        if (input == stdin && isatty(STDIN_FILENO)) {
            printf("myshell> ");
            fflush(stdout);
        }

        errno = 0;

        if (getline(&line, &size, input) == -1) {
            if (errno == EINTR) {
                clearerr(input);

                if (show_history) {
                    show_history = 0;
                    print_history();
                }

                continue;
            }

            break;
        }

        line[strcspn(line, "\n")] = '\0';

        if (line[0] == '\0') {
            continue;
        }

        if (strncmp(line, "#!", 2) == 0) {
            continue;
        }

        char original[MAX_LINE];
        safe_copy(original, line, sizeof(original));

        add_history(original);

        if (use_syslog) {
            syslog(LOG_INFO, "command: %s", original);
        }

        char copy[MAX_LINE];
        safe_copy(copy, line, sizeof(copy));

        char *args[MAX_ARGS];
        int arg_count = split_line(copy, args);

        if (arg_count == 0) {
            continue;
        }

        int background = 0;

        if (strcmp(args[arg_count - 1], "&") == 0) {
            background = 1;
            args[arg_count - 1] = NULL;
            arg_count--;
        }

        if (arg_count == 0) {
            continue;
        }

        char *outfile = NULL;
        int append = 0;

        if (find_redirect(args, &arg_count, &outfile, &append) < 0) {
            continue;
        }

        if (arg_count == 0) {
            fprintf(stderr, "missing command\n");
            continue;
        }

        if (builtin_cd(args, arg_count)) {
            continue;
        }

        if (strcmp(args[0], "jobs") == 0) {
            print_jobs();
            continue;
        }

        run_command(args, background, original, outfile, append);
    }

    free(line);

    if (input != stdin) {
        fclose(input);
    }

    if (use_syslog) {
        closelog();
    }

    return 0;
}
