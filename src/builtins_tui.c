#include "builtins_internal.h"

#include <signal.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <poll.h>
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
/* UTF-8 byte queue fed from console key records.
 *
 * ReadConsoleInputW reports UTF-16 code units in UnicodeChar, independent of
 * the console input code page. The queue holds their UTF-8 encoding, so
 * read-byte is byte-exact for any character, and terminal-input-available-p
 * and terminal-wait agree with what read-byte will return. These are plain C
 * statics holding no Lisp objects, so the GC never needs to scan them. */
#define WIN_INPUT_QUEUE_CAP 256

static unsigned char win_input_queue[WIN_INPUT_QUEUE_CAP];
static size_t win_input_head = 0;
static size_t win_input_tail = 0;
static uint16_t win_pending_high = 0;

/* Sentinel results for win_next_byte (bytes are 0..255). */
#define WIN_NEXT_TIMEOUT (-1)
#define WIN_NEXT_EOF     (-2)

static size_t win_queue_count(void)
{
    return win_input_tail - win_input_head;
}

static int win_queue_push(const char *bytes, int count)
{
    if (count <= 0)
        return 1;
    if ((size_t)count > WIN_INPUT_QUEUE_CAP - win_queue_count())
        return 0;

    if (win_input_tail + (size_t)count > WIN_INPUT_QUEUE_CAP) {
        memmove(win_input_queue, win_input_queue + win_input_head, win_queue_count());
        win_input_tail -= win_input_head;
        win_input_head = 0;
    }
    memcpy(win_input_queue + win_input_tail, bytes, (size_t)count);
    win_input_tail += (size_t)count;
    return 1;
}

static int win_queue_pop(unsigned char *out)
{
    if (win_input_head == win_input_tail)
        return 0;
    *out = win_input_queue[win_input_head++];
    if (win_input_head == win_input_tail)
        win_input_head = win_input_tail = 0;
    return 1;
}

/* Queue the bytes of every character key-down record, discarding records that
 * can never produce a byte. WINDOW_BUFFER_SIZE_EVENT records set winch_flag.
 * Returns 1 when a byte is queued, 0 when nothing is available (or stdin is
 * not a console). */
static int win_input_pending(void)
{
    init_tui_handles();
    if (tui_stdin_handle == INVALID_HANDLE_VALUE)
        return 0;

    DWORD mode;
    if (!GetConsoleMode(tui_stdin_handle, &mode))
        return 0;

    for (;;) {
        if (win_queue_count() > 0)
            return 1;

        DWORD available = 0;
        if (!GetNumberOfConsoleInputEvents(tui_stdin_handle, &available) || available == 0)
            return 0;

        INPUT_RECORD record;
        DWORD consumed = 0;
        if (!ReadConsoleInputW(tui_stdin_handle, &record, 1, &consumed) || consumed == 0)
            return 0;

        if (record.EventType == WINDOW_BUFFER_SIZE_EVENT) {
            winch_flag = 1;
            continue;
        }
        if (record.EventType != KEY_EVENT || !record.Event.KeyEvent.bKeyDown)
            continue;

        uint16_t unit = record.Event.KeyEvent.uChar.UnicodeChar;
        if (unit == 0)
            continue;

        DWORD repeat = record.Event.KeyEvent.wRepeatCount;
        if (repeat == 0)
            repeat = 1;
        for (DWORD i = 0; i < repeat; i++) {
            char bytes[8];
            int n = utf8_encode_utf16_unit(unit, &win_pending_high, bytes);
            if (n == 0)
                continue; /* a high surrogate, held for the next record */
            if (!win_queue_push(bytes, n))
                break; /* repeat overflow: clamp to the queue capacity */
        }
    }
}

/* Return the next byte, WIN_NEXT_TIMEOUT, or WIN_NEXT_EOF. A timeout below 0
 * waits forever. With redirected stdin this reads bytes with ReadFile (no
 * timeout support: the read blocks until data or EOF). */
static int win_next_byte(long long timeout_ms)
{
    unsigned char byte;
    if (win_queue_pop(&byte))
        return (int)byte;

    init_tui_handles();
    if (tui_stdin_handle == INVALID_HANDLE_VALUE)
        return WIN_NEXT_EOF;

    DWORD mode;
    if (!GetConsoleMode(tui_stdin_handle, &mode)) {
        DWORD read = 0;
        if (!ReadFile(tui_stdin_handle, &byte, 1, &read, NULL) || read == 0)
            return WIN_NEXT_EOF;
        return (int)byte;
    }

    ULONGLONG start = GetTickCount64();
    for (;;) {
        if (win_input_pending() && win_queue_pop(&byte))
            return (int)byte;

        DWORD wait_ms = INFINITE;
        if (timeout_ms >= 0) {
            ULONGLONG elapsed = GetTickCount64() - start;
            if (elapsed >= (ULONGLONG)timeout_ms)
                return WIN_NEXT_TIMEOUT;
            wait_ms = (DWORD)((ULONGLONG)timeout_ms - elapsed);
        }

        if (WaitForSingleObject(tui_stdin_handle, wait_ms) == WAIT_TIMEOUT)
            return WIN_NEXT_TIMEOUT;
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
    available = win_input_pending();
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
    win_input_pending();
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
        if (win_input_pending())
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
    int ch = win_next_byte(-1);
    if (ch < 0)
        return NIL;
    return lisp_make_integer(ch);
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
    int ch = win_next_byte(ms);
    if (ch < 0)
        return NIL;
    return lisp_make_integer(ch);
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

/* ========================================================================== */
/* Raw output                                                                 */
/* ========================================================================== */

/* Write all LEN bytes of BUF to stdout, bypassing stdio. Returns 0 on
 * success, or -1 with errno set. Retries on EINTR and short writes, and
 * waits for the descriptor when another process left the tty non-blocking. */
static int write_all_stdout(const char *buf, size_t len)
{
    int rc = 0;

#ifdef _WIN32
    /* fd 1 is in text mode, where _write turns every "\n" into "\r\n" and so
     * makes the output one byte longer per line. Write the bytes as they are
     * and put the descriptor back the way it was found, so princ and format
     * keep their text-mode translation. */
    int prev_mode = _setmode(1, _O_BINARY);
#endif

    while (len > 0) {
#ifdef _WIN32
        unsigned int chunk = len > INT_MAX ? INT_MAX : (unsigned int)len;
        int n = _write(1, buf, chunk);
        if (n < 0) {
            rc = -1;
            break;
        }
#else
        ssize_t n = write(STDOUT_FILENO, buf, len);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                struct pollfd pfd = { STDOUT_FILENO, POLLOUT, 0 };
                (void)poll(&pfd, 1, -1);
                continue;
            }
            return -1;
        }
#endif
        if (n == 0) {
            errno = EIO;
            rc = -1;
            break;
        }
        buf += n;
        len -= (size_t)n;
    }

#ifdef _WIN32
    if (prev_mode != -1)
        (void)_setmode(1, prev_mode);
#endif

    return rc;
}

/* terminal-write: write a string to stdout without stdio's line buffering, so
 * a whole frame goes out in one write(2) however long it is or whatever
 * newlines it holds. Returns the string, like princ. */
static LispObject *builtin_terminal_write(LispObject *args, Environment *env)
{
    (void)env;

    if (args == NIL || LISP_CDR(args) != NIL)
        return lisp_raise_error("terminal-write requires 1 argument");

    LispObject *arg = LISP_CAR(args);
    if (LISP_TYPE(arg) != LISP_STRING)
        return lisp_raise_error("terminal-write: argument must be a string");

    const char *text = LISP_STR_VAL(arg);

    /* Anything princ or format left in the stdio buffer goes out first. */
    fflush(stdout);
    if (write_all_stdout(text, strlen(text)) != 0)
        return lisp_raise_error("terminal-write: write failed");

    return arg;
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
    REGISTER("terminal-write", builtin_terminal_write);
}
