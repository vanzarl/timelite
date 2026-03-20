#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>

#define CLOCK_FPS 0.5 // for updating clock
#define FPS 2 // for reading user's keys

int running = 1;

struct termios old_termios, new_termios;
int old_flags;

void reset_terminal()
{
    fcntl(STDIN_FILENO, F_SETFL, old_flags); // return state of read's blocking

    printf("\e[m"); // reset color changes
    printf("\e[?25h"); // show cursor

    fflush(stdout);

    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
}

void configure_terminal()
{
    old_flags = fcntl(STDIN_FILENO, F_GETFL); // read doesn't block
    fcntl(STDIN_FILENO, F_SETFL, old_flags | O_NONBLOCK);

    tcgetattr(STDIN_FILENO, &old_termios);
    new_termios = old_termios;

    new_termios.c_lflag &= ~(ICANON | ECHO); // disable canonical and echo
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

    printf("\e[?25l"); // hide cursor
    atexit(reset_terminal);
}

typedef enum {
    CLOCK_MODE = 0,
    TIMER_MODE,
    STOPWATCH_MODE,
} Mode;

typedef struct {
    Mode mode;
    struct timespec time_start;
    struct timespec time_end; // when timer will end
} State;

void gettime(struct timespec *ts)
{
    clock_gettime(CLOCK_MONOTONIC, ts);
}

struct timespec ms_to_timespec(int64_t ms)
{
    struct timespec ts;

    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;

    return ts;
}

#define NSEC_PER_SEC 1000000000L // 10e9 nsec = 1 sec

struct timespec timespec_diff(const struct timespec *start, const struct timespec *end)
{
    struct timespec diff;

    diff.tv_sec = end->tv_sec - start->tv_sec;
    diff.tv_nsec = end->tv_nsec - start->tv_nsec;

    if (diff.tv_nsec < 0) {
        diff.tv_sec--;
        diff.tv_nsec += NSEC_PER_SEC;
    }

    return diff;
}

struct timespec timespec_add(const struct timespec *ts1, const struct timespec *ts2)
{
    struct timespec add;

    add.tv_sec = ts1->tv_sec + ts2->tv_sec;
    add.tv_nsec = ts1->tv_nsec + ts2->tv_nsec;

    if (add.tv_nsec > NSEC_PER_SEC) {
        add.tv_sec++;
        add.tv_nsec -= NSEC_PER_SEC;
    }

    return add;
}

void update_clock(State *state)
{
    (void)state;
    time_t now_raw = time(NULL);
    struct tm now = *localtime(&now_raw);
    printf("\r\e[K%02d:%02d:%02d", now.tm_hour, now.tm_min, now.tm_sec);
    fflush(stdout);
}

void update_stopwatch(State *state)
{
    struct timespec now;
    gettime(&now);

    struct timespec diff = timespec_diff(&state->time_start, &now);

    int hours = diff.tv_sec / 3600;
    int minutes = (diff.tv_sec % 3600) / 60;
    int seconds = diff.tv_sec % 60;

    printf("\r\e[K%02d:%02d:%02d", hours, minutes, seconds);
    fflush(stdout);
}

void update_timer(State *state)
{
    struct timespec now;
    gettime(&now);

    struct timespec diff = timespec_diff(&now, &state->time_end);

    if (diff.tv_sec <= 0) {
        printf("\r\e[K00:00:00 Timer ended!\n");
        fflush(stdout);
        exit(0);
        return;
    }

    int hours = diff.tv_sec / 3600;
    int minutes = (diff.tv_sec % 3600) / 60;
    int seconds = diff.tv_sec % 60;

    printf("\r\033[K%02d:%02d:%02d", hours, minutes, seconds);
    fflush(stdout);
}

void print_help(void)
{
    printf("Usage:\n");
    printf("  -c                     clock mode\n");
    printf("  -t [HH:MM:SS|MM:SS|SS] timer mode\n");
    printf("  -s                     stopwatch mode\n");
    printf("  -n                     name (string before clock/timer/stopwatch)\n");
    printf("  -h                     help (this message)\n");
}

int count_char(const char *data, char symbol)
{
    int count = 0;

    for (int i = 0; data[i] != '\0'; i++) {
        if (data[i] == symbol) {
            // printf("data[i]: %c; symbol: %c\n", data[i], symbol);
            count++;
        }
    }

    return count;
}

int string_to_time_t(const char *string) // string must be in format [HH:MM:SS|MM:SS|SS]
{
    char *string_time = malloc(sizeof(char) * (strlen(string) + 1));
    strcpy(string_time, optarg);

    int count_colon = count_char(string_time, ':');

    time_t seconds = 0;
    int i = 2 - count_colon; // 2 because the largest variant is HH:MM:SS

    char *token;

    while ((token = strsep(&string_time, ":")) != NULL) {
        int _value = !strcmp(token, "") ? 0 : atoi(token);

        if (_value < 0) {
            printf("Argument < 0!\n");
            exit(EXIT_FAILURE);
        }

        time_t value = _value;

        switch (i) {
        case 0:
            value *= 3600; // convert hours into seconds
            break;
        case 1:
            value *= 60; // convert minutes into seconds
            break;
        case 2:
            break;
        default:
            printf("Ivalid time format!\n");
            exit(EXIT_FAILURE);
        }

        i++;
        seconds += value;
    }

    free(string_time);

    return seconds;
}

#define AVALIABLE_FLAGS "ct:sn:h"

void validate_flags(int argc, char *argv[])
{
    int flags = 0;

    int opt;
    while ((opt = getopt(argc, argv, AVALIABLE_FLAGS)) != -1) {
        switch (opt) {
        case 'c':
        case 't':
        case 's':
            flags++;
            break;
        }
    }

    if (flags > 1) {
        printf("Flags c, t, s can't use together!\n");
        exit(EXIT_FAILURE);
    }

    optind = 1;
}

void parse_flags(int argc, char *argv[], State *state)
{
    int opt;
    while ((opt = getopt(argc, argv, AVALIABLE_FLAGS)) != -1) {
        switch (opt) {
        case 'c':
            state->mode = CLOCK_MODE;
            break;
        case 't':
            state->mode = TIMER_MODE;

            gettime(&state->time_start);
            state->time_end = state->time_start;

            state->time_end.tv_sec += string_to_time_t(optarg);

            break;
        case 's':
            state->mode = STOPWATCH_MODE;
            gettime(&state->time_start);
            break;
        case 'n':
            printf("%s\n", optarg);
            break;
        case 'h':
            print_help();
            exit(0);
        case '?':
            exit(EXIT_FAILURE);
        case ':':
            exit(EXIT_FAILURE);
        }
    }
}

void handle_input()
{
    char c;
    ssize_t length_buffer = read(STDIN_FILENO, &c, sizeof(c));

    if (length_buffer > 0) {
        switch (c) {
        case 'q':
            printf("\n");
            exit(0);
        }
    }
}

void signal_handler(int sig)
{
    (void)sig;
    printf("\n");
    exit(0);
}

void setup_handling_signal()
{
    struct sigaction sa = {0};

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask); // don't block other signals
    sa.sa_flags = SA_RESTART; // system calls (read, sleep) automatically continue after a signal

    sigaction(SIGINT, &sa, NULL);
}

int main(int argc, char *argv[])
{
    configure_terminal();

    setup_handling_signal(); // handle Ctrl+C

    State state = {0};

    validate_flags(argc, argv);
    parse_flags(argc, argv, &state); // set state

    void (*update)(State *state);

    switch (state.mode) {
    case CLOCK_MODE:
        update = update_clock;
        break;

    case TIMER_MODE:
        update = update_timer;
        break;

    case STOPWATCH_MODE:
        update = update_stopwatch;
        break;
    }

    struct timespec time_frame = ms_to_timespec(1000.0 / FPS);

    struct timespec time_frame_clock = ms_to_timespec(1000.0 / CLOCK_FPS);
    struct timespec accumulator = time_frame_clock;

    struct timespec time_start;
    gettime(&time_start);

    while (running) {

        handle_input();

        accumulator = timespec_add(&accumulator, &time_frame);
        struct timespec diff = timespec_diff(&time_frame_clock, &accumulator);
        if (diff.tv_sec >= 0 && diff.tv_nsec >= 0) {
            update(&state);
            accumulator = diff;
        }

        // evaluate time to sleep

        struct timespec time_end;
        gettime(&time_end);

        struct timespec dt = timespec_diff(&time_start, &time_end); // time that program works

        struct timespec delay = timespec_diff(&dt, &time_frame); // time to sleep

        if (delay.tv_sec >= 0 && delay.tv_nsec > 0) {
            nanosleep(&delay, NULL);
        }

        gettime(&time_start);
    }

    return 0;
}
