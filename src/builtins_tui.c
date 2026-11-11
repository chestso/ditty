#include "builtins_internal.h"

#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#include <io.h>
#else
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#endif

/* ========================================================================== */
/* Sleep                                                                      */
/* ========================================================================== */

static LispObject *builtin_sleep(LispObject *args, Environment *env)
{
    (void)env;

    if (args == NIL)
        return lisp_raise_error("sleep requires 1 argument");

    LispObject *arg = LISP_CAR(args);
    if (LISP_TYPE(arg) != LISP_INTEGER)
        return lisp_raise_error("sleep: argument must be an integer");

    long long ms = LISP_INT_VAL(arg);
    if (ms < 0)
        return lisp_raise_error("sleep: duration cannot be negative");

#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif

    return LISP_TRUE;
}

/* ========================================================================== */
/* Terminal state                                                             */
/* ========================================================================== */

static volatile sig_atomic_t winch_flag = 1;

#ifdef _WIN32
static HANDLE tui_stdin_handle = INVALID_HANDLE_VALUE;
static DWORD saved_in_mode = 0;
static DWORD saved_out_mode = 0;
static int raw_mode_saved = 0;

static void init_tui_handles(void)
{
    if (tui_stdin_handle == INVALID_HANDLE_VALUE)
        tui_stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
}

static int is_tty_input(void)
{
    init_tui_handles();
    if (tui_stdin_handle == INVALID_HANDLE_VALUE)
        return 0;
    DWORD mode;
    return GetConsoleMode(tui_stdin_handle, &mode) != 0;
}
#else
static struct termios saved_termios;
static int raw_mode_saved = 0;

static int is_tty_input(void)
{
    return isatty(STDIN_FILENO);
}
#endif

#ifdef _WIN32
/* terminal-tty-p needs both streams to be terminals. */
static int is_tty_output(void)
{
    HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    if (h_out == INVALID_HANDLE_VALUE)
        return 0;
    return GetConsoleMode(h_out, &mode) != 0;
}
#else
static int is_tty_output(void)
{
    return isatty(STDOUT_FILENO);
}
#endif

/* ========================================================================== */
/* Console event plumbing                                                     */
/* ========================================================================== */

#ifdef _WIN32
/* Returns 1 if a key-down record carrying a character is queued (the next
 * read-byte will not block), else 0. Discards, as it scans, every record
 * that can never produce a byte; WINDOW_BUFFER_SIZE_EVENT records set
 * winch_flag first. Returns 0 when stdin is not a console. */
static int win_console_input_pending(void)
{
    init_tui_handles();
    if (tui_stdin_handle == INVALID_HANDLE_VALUE)
        return 0;

    DWORD mode;
    if (!GetConsoleMode(tui_stdin_handle, &mode))
        return 0;

    for (;;) {
        DWORD available = 0;
        if (!GetNumberOfConsoleInputEvents(tui_stdin_handle, &available) || available == 0)
            return 0;

        DWORD to_peek = available < 128 ? available : 128;
        INPUT_RECORD records[128];
        DWORD peeked = 0;
        if (!PeekConsoleInputW(tui_stdin_handle, records, to_peek, &peeked) || peeked == 0)
            return 0;

        DWORD match = peeked;
        int found = 0;
        for (DWORD i = 0; i < peeked; i++) {
            if (records[i].EventType == KEY_EVENT && records[i].Event.KeyEvent.bKeyDown &&
                records[i].Event.KeyEvent.uChar.UnicodeChar != 0) {
                match = i;
                found = 1;
                break;
            }
        }

        if (match > 0) {
            DWORD consumed = 0;
            if (ReadConsoleInputW(tui_stdin_handle, records, match, &consumed)) {
                for (DWORD i = 0; i < consumed; i++) {
                    if (records[i].EventType == WINDOW_BUFFER_SIZE_EVENT)
                        winch_flag = 1;
                }
            }
        }

        if (found)
            return 1;
        /* Nothing readable in this batch: more records may sit beyond it. */
    }
}
#else
static int winch_pipe[2] = { -1, -1 };

static void sigwinch_handler(int sig)
{
    (void)sig;
    winch_flag = 1;
    if (winch_pipe[1] >= 0) {
        unsigned char byte = 1;
        ssize_t written = write(winch_pipe[1], &byte, 1);
        (void)written; /* EAGAIN just means a byte is already pending */
    }
}

/* Install the SIGWINCH handler (and its self-pipe) exactly once. */
static void ensure_winch_handler(void)
{
    static int installed = 0;
    if (installed)
        return;

    if (winch_pipe[0] < 0 && winch_pipe[1] < 0) {
        if (pipe(winch_pipe) == 0) {
            for (int i = 0; i < 2; i++) {
                int flags = fcntl(winch_pipe[i], F_GETFL, 0);
                if (flags >= 0)
                    fcntl(winch_pipe[i], F_SETFL, flags | O_NONBLOCK);
                fcntl(winch_pipe[i], F_SETFD, FD_CLOEXEC);
            }
        } else {
            winch_pipe[0] = -1;
            winch_pipe[1] = -1;
        }
    }

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = sigwinch_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0; /* no SA_RESTART: select() must see EINTR */
    sigaction(SIGWINCH, &action, NULL);

    installed = 1;
}

/* Drop any pending self-pipe bytes. */
static void drain_winch_pipe(void)
{
    if (winch_pipe[0] < 0)
        return;
    unsigned char buffer[64];
    while (read(winch_pipe[0], buffer, sizeof(buffer)) > 0) {
    }
}
#endif

/* set-terminal-raw: disable line buffering and local echo on stdin. */
static LispObject *builtin_set_terminal_raw(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;

    if (!is_tty_input())
        return LISP_TRUE;

    if (raw_mode_saved)
        return LISP_TRUE;

#ifdef _WIN32
    init_tui_handles();
    if (GetConsoleMode(tui_stdin_handle, &saved_in_mode)) {
        DWORD new_in = saved_in_mode;
        new_in &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
        new_in |= ENABLE_VIRTUAL_TERMINAL_INPUT;
        if (SetConsoleMode(tui_stdin_handle, new_in))
            raw_mode_saved = 1;
    }
    HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h_out != INVALID_HANDLE_VALUE && GetConsoleMode(h_out, &saved_out_mode)) {
        DWORD new_out = saved_out_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(h_out, new_out);
    }
#else
    if (tcgetattr(STDIN_FILENO, &saved_termios) != 0)
        return LISP_TRUE;

    struct termios raw = saved_termios;
    raw.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    raw.c_cflag &= ~(CSIZE | PARENB);
    raw.c_cflag |= CS8;
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0)
        raw_mode_saved = 1;
#endif

    return LISP_TRUE;
}

/* restore-terminal: restore the terminal state saved by set-terminal-raw. */
static LispObject *builtin_restore_terminal(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;

    if (!raw_mode_saved)
        return LISP_TRUE;

#ifdef _WIN32
    if (tui_stdin_handle != INVALID_HANDLE_VALUE)
        SetConsoleMode(tui_stdin_handle, saved_in_mode);
    HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h_out != INVALID_HANDLE_VALUE)
        SetConsoleMode(h_out, saved_out_mode);
#else
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_termios);
#endif

    raw_mode_saved = 0;
    return LISP_TRUE;
}

/* terminal-size: return the current terminal size as (rows . cols). */
static LispObject *builtin_terminal_size(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;

    int rows = 0;
    int cols = 0;

#ifdef _WIN32
    HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (h_out != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(h_out, &csbi)) {
        rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 ||
        ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0) {
        rows = ws.ws_row;
        cols = ws.ws_col;
    }
#endif

    if (rows > 0 && cols > 0)
        return lisp_make_cons(lisp_make_integer(rows), lisp_make_integer(cols));
    return NIL;
}

/* terminal-pixel-size: return the text area size in pixels as (width . height). */
static LispObject *builtin_terminal_pixel_size(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;

#ifdef _WIN32
    /* Windows consoles expose no portable pixel dimensions */
    return NIL;
#else
    struct winsize ws;
    int ok = ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0;
    if (!ok)
        ok = ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0;

    if (ok && ws.ws_xpixel > 0 && ws.ws_ypixel > 0)
        return lisp_make_cons(lisp_make_integer(ws.ws_xpixel), lisp_make_integer(ws.ws_ypixel));
    return NIL;
#endif
}

/* terminal-input-available-p: return #t if stdin has data ready to read. */
static LispObject *builtin_terminal_input_available_p(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;

    int available = 0;

#ifdef _WIN32
    available = win_console_input_pending();
#else
    fd_set fds;
    struct timeval tv;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0)
        available = 1;
#endif

    return lisp_make_boolean(available);
}

/* terminal-tty-p: return #t if stdin and stdout are both terminals. */
static LispObject *builtin_terminal_tty_p(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;

    return lisp_make_boolean(is_tty_input() && is_tty_output());
}

/* terminal-resized-p: return #t once after each terminal resize. */
static LispObject *builtin_terminal_resized_p(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;

#ifdef _WIN32
    /* Pick up WINDOW_BUFFER_SIZE_EVENT records before looking at the flag. */
    win_console_input_pending();
#else
    ensure_winch_handler();
#endif

    if (winch_flag) {
        winch_flag = 0;
#ifndef _WIN32
        drain_winch_pipe();
#endif
        return LISP_TRUE;
    }
    return NIL;
}

/* terminal-wait: block until input, a resize, or the timeout (ms). Returns
 * one of the symbols resize, input, timeout. */
static LispObject *builtin_terminal_wait(LispObject *args, Environment *env)
{
    (void)env;

    /* -1 means "no timeout". */
    long long timeout = -1;

    if (args != NIL) {
        LispObject *arg = LISP_CAR(args);
        if (arg != NIL) {
            if (LISP_TYPE(arg) != LISP_INTEGER)
                return lisp_raise_error("terminal-wait: timeout must be a non-negative integer, or nil");
            timeout = LISP_INT_VAL(arg);
            if (timeout < 0)
                return lisp_raise_error("terminal-wait: timeout cannot be negative");
        }
    }

#ifdef _WIN32
    init_tui_handles();
    DWORD console_mode;
    if (tui_stdin_handle == INVALID_HANDLE_VALUE ||
        !GetConsoleMode(tui_stdin_handle, &console_mode)) {
        /* Redirected stdin: the caller's read-byte observes data or EOF. */
        return lisp_intern("input");
    }

    ULONGLONG start = GetTickCount64();
    for (;;) {
        if (win_console_input_pending())
            return lisp_intern("input");
        if (winch_flag) {
            winch_flag = 0;
            return lisp_intern("resize");
        }

        DWORD wait_ms = INFINITE;
        if (timeout >= 0) {
            ULONGLONG elapsed = GetTickCount64() - start;
            if (elapsed >= (ULONGLONG)timeout)
                return lisp_intern("timeout");
            wait_ms = (DWORD)((ULONGLONG)timeout - elapsed);
        }

        DWORD waited = WaitForSingleObject(tui_stdin_handle, wait_ms);
        if (waited == WAIT_TIMEOUT)
            return lisp_intern("timeout");
        /* Signalled by input (possibly only a discarded record): re-check. */
    }
#else
    ensure_winch_handler();

    for (;;) {
        if (winch_flag) {
            winch_flag = 0;
            drain_winch_pipe();
            return lisp_intern("resize");
        }

        struct timespec start;
        clock_gettime(CLOCK_MONOTONIC, &start);

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        int maxfd = STDIN_FILENO;
        if (winch_pipe[0] >= 0) {
            FD_SET(winch_pipe[0], &fds);
            if (winch_pipe[0] > maxfd)
                maxfd = winch_pipe[0];
        }

        struct timeval tv;
        struct timeval *tvp = NULL;
        if (timeout >= 0) {
            tv.tv_sec = timeout / 1000;
            tv.tv_usec = (timeout % 1000) * 1000;
            tvp = &tv;
        }

        int ready = select(maxfd + 1, &fds, NULL, NULL, tvp);
        if (ready == 0)
            return lisp_intern("timeout");
        if (ready < 0) {
            if (errno != EINTR)
                return lisp_raise_error("terminal-wait: select failed");
            /* Recompute what is left of the timeout. */
            if (timeout >= 0) {
                struct timespec now;
                clock_gettime(CLOCK_MONOTONIC, &now);
                long long elapsed = (long long)(now.tv_sec - start.tv_sec) * 1000 +
                                    (now.tv_nsec - start.tv_nsec) / 1000000;
                if (elapsed < 0)
                    elapsed = 0;
                timeout -= elapsed;
                if (timeout <= 0)
                    return lisp_intern("timeout");
            }
            continue;
        }

        if (winch_pipe[0] >= 0 && FD_ISSET(winch_pipe[0], &fds)) {
            winch_flag = 0;
            drain_winch_pipe();
            return lisp_intern("resize");
        }
        if (FD_ISSET(STDIN_FILENO, &fds))
            return lisp_intern("input");
    }
#endif
}

/* ========================================================================== */
/* Raw input                                                                  */
/* ========================================================================== */

/* read-byte: read a single byte from stdin. Returns nil on EOF. */
static LispObject *builtin_read_byte(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;

#ifdef _WIN32
    int ch = _getch();
    if (ch == EOF)
        return NIL;
    return lisp_make_integer(ch & 0xFF);
#else
    unsigned char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n <= 0)
        return NIL;
    return lisp_make_integer(c);
#endif
}

/* read-byte-timeout: read a single byte from stdin with a timeout in ms.
   Returns nil if no input is available before the timeout expires. */
static LispObject *builtin_read_byte_timeout(LispObject *args, Environment *env)
{
    (void)env;

    if (args == NIL)
        return lisp_raise_error("read-byte-timeout requires 1 argument");

    LispObject *arg = LISP_CAR(args);
    if (LISP_TYPE(arg) != LISP_INTEGER)
        return lisp_raise_error("read-byte-timeout: argument must be an integer");

    long long ms = LISP_INT_VAL(arg);
    if (ms < 0)
        return lisp_raise_error("read-byte-timeout: timeout cannot be negative");

#ifdef _WIN32
    long long elapsed = 0;
    while (elapsed < ms) {
        if (_kbhit()) {
            int ch = _getch();
            if (ch == EOF)
                return NIL;
            return lisp_make_integer(ch & 0xFF);
        }
        Sleep(1);
        elapsed++;
    }
    return NIL;
#else
    fd_set fds;
    struct timeval tv;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;

    if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) <= 0)
        return NIL;

    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) <= 0)
        return NIL;
    return lisp_make_integer(c);
#endif
}

void register_tui_builtins(Environment *env)
{
    REGISTER("sleep", builtin_sleep);
    REGISTER("set-terminal-raw", builtin_set_terminal_raw);
    REGISTER("restore-terminal", builtin_restore_terminal);
    REGISTER("terminal-size", builtin_terminal_size);
    REGISTER("terminal-pixel-size", builtin_terminal_pixel_size);
    REGISTER("terminal-tty-p", builtin_terminal_tty_p);
    REGISTER("terminal-wait", builtin_terminal_wait);
    REGISTER("terminal-input-available-p", builtin_terminal_input_available_p);
    REGISTER("terminal-resized-p", builtin_terminal_resized_p);
    REGISTER("read-byte", builtin_read_byte);
    REGISTER("read-byte-timeout", builtin_read_byte_timeout);
}
