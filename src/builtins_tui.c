#include "builtins_internal.h"

#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#include <io.h>
#else
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
        return lisp_make_error("sleep requires 1 argument");

    LispObject *arg = LISP_CAR(args);
    if (LISP_TYPE(arg) != LISP_INTEGER)
        return lisp_make_error("sleep: argument must be an integer");

    long long ms = LISP_INT_VAL(arg);
    if (ms < 0)
        return lisp_make_error("sleep: duration cannot be negative");

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

/* terminal-input-available-p: return #t if stdin has data ready to read. */
static LispObject *builtin_terminal_input_available_p(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;

    int available = 0;

#ifdef _WIN32
    init_tui_handles();
    DWORD count = 0;
    if (tui_stdin_handle != INVALID_HANDLE_VALUE &&
        GetNumberOfConsoleInputEvents(tui_stdin_handle, &count))
        available = count > 0;
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

#ifndef _WIN32
static void sigwinch_handler(int sig)
{
    (void)sig;
    winch_flag = 1;
}
#endif

/* terminal-resized-p: return #t once after each terminal resize. */
static LispObject *builtin_terminal_resized_p(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;

#ifndef _WIN32
    static int handler_installed = 0;
    if (!handler_installed) {
        signal(SIGWINCH, sigwinch_handler);
        handler_installed = 1;
    }
#endif

    if (winch_flag) {
        winch_flag = 0;
        return LISP_TRUE;
    }
    return NIL;
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
        return lisp_make_error("read-byte-timeout requires 1 argument");

    LispObject *arg = LISP_CAR(args);
    if (LISP_TYPE(arg) != LISP_INTEGER)
        return lisp_make_error("read-byte-timeout: argument must be an integer");

    long long ms = LISP_INT_VAL(arg);
    if (ms < 0)
        return lisp_make_error("read-byte-timeout: timeout cannot be negative");

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
    REGISTER("terminal-input-available-p", builtin_terminal_input_available_p);
    REGISTER("terminal-resized-p", builtin_terminal_resized_p);
    REGISTER("read-byte", builtin_read_byte);
    REGISTER("read-byte-timeout", builtin_read_byte_timeout);
}
