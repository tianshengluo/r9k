/* rsh - remote shell */
/*
 * readline.c - 终端行输入实现
 *
 * UTF-8 终端输入实现，无第三方依赖。
 * 实现常用字符编辑、光标移动与缓冲区索引同步。
 * 支持主流 ESC 控制序列。
 *
 * author: Varketh Nockrath
 * created: 2025/10/24
 */
#include <r9k/readline.h>
#include <wchar.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <signal.h>

#define LINE_BUFF_MAX 16384
#define HISTORY_MAX 256

static struct termios orig;

struct history
{
        char** lines;
        char* buf;
        size_t len;
        size_t max;
};

struct termline
{
        char* bufptr;         /* 正在编辑行缓冲区 */
        char* saveptr;        /* 用户新编辑行缓冲 */
        int   start;          /* prompt 列宽 */
        int   cursor;         /* 光标所在位置 */
        int   maxcol;         /* 当前行最大列宽 */
        int   index;          /* 光标对应的缓冲区字符起始索引位置 */
        int   len;            /* 缓冲区长度 */
        int   hidx;           /* 历史记录索引 */
        struct history *snap; /* 临时历史记录快照 */
};

static struct history *hist = NULL;

static struct history *history_create(void)
{
        struct history *history;

        history = (struct history *) calloc(1, sizeof(struct history));
        history->max = HISTORY_MAX;

        history->lines = (char **) calloc(history->max, sizeof(char *));
        history->buf = (char *) calloc(history->max, LINE_BUFF_MAX);

        for (size_t i = 0; i < history->max; i++)
                history->lines[i] = history->buf + (i * LINE_BUFF_MAX);

        return history;
}

static struct history *history_copy(const struct history *orig)
{
        struct history *copy = history_create();
        copy->len = orig->len;
        copy->max = orig->len;

        memcpy(copy->buf, orig->buf, orig->max * LINE_BUFF_MAX);
        for (size_t i = 0; i < orig->len; i++)
                copy->lines[i] = copy->buf + (i * LINE_BUFF_MAX);

        return copy;
}

static void history_free(struct history *history)
{
        if (!history)
                return;

        if (history->lines) {
                free(history->buf);
                free(history->lines);
        }

        free(history);
}

static void set_raw_mode(int hint)
{
        if (hint) {
                struct termios raw;
                raw = orig;
                raw.c_lflag &= ~(ICANON | ECHO);
                tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        } else {
                tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
        }
}

static void raw_on(void)
{
        set_raw_mode(1);
}

static void raw_off(void)
{
        set_raw_mode(0);
}

static void signal_handler(int sig)
{
        (void) sig;
        raw_off();
        exit(0);
}

__attribute__((unused))
static void setup_signal_handler(void)
{
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
        signal(SIGHUP, signal_handler);
}

__attribute__((constructor))
static void init(void)
{
        tcgetattr(STDIN_FILENO, &orig);
        setlocale(LC_CTYPE, "");
        setvbuf(stdout, NULL, _IONBF, 0);
        atexit(raw_off);
        setup_signal_handler();
        hist = history_create();
}

/* 重绘一行文本 */
inline static void redraw(const struct termline *line)
{
        printf("\x1b[%dG\x1b[K", line->start);
        write(STDOUT_FILENO, line->bufptr, line->len);
        printf("\x1b[%dG", line->start + line->cursor);
}

/* UTF-8 特征：
 *   二进制 10xxxxxx 表示为 UTF-8 编码的续字节。
 *   非二进制 10xxxxxx 开头表示为 UTF-8 新字符起始字符。
 */
inline static int utf8lead(unsigned char uc)
{
        return (uc & 0xC0) != 0x80;
}

inline static int utf8len(unsigned char uc)
{
        if ((uc & 0x80) == 0x00) return 1;
        if ((uc & 0xE0) == 0xC0) return 2;
        if ((uc & 0xF0) == 0xE0) return 3;
        if ((uc & 0xF8) == 0xF0) return 4;
        return 1;
}

inline static int utf8width(const char *buf)
{
        wchar_t wc;
        mbstate_t mbs;
        memset(&mbs, 0, sizeof(mbs));

        int r = mbrtowc(&wc, buf, MB_CUR_MAX, &mbs);
        if (r == -1 || r == -2)
                return 1;

        int w = wcwidth(wc);
        if (w < 0)
                w = 1;

        return w;
}

inline static int utf8wtotal(const char *buf)
{
        int wtotal = 0;

        wchar_t wc;
        mbstate_t mbs;
        const char *p = buf;
        memset(&mbs, 0, sizeof(mbs));
        size_t len = strlen(buf);

        while (len > 0) {
                int r = mbrtowc(&wc, p, MB_CUR_MAX, &mbs);
                if (r == -1 || r == -2) {
                        wtotal += 1;
                        p += 1;
                        len -= 1;
                        continue;
                }

                if (r == 0)
                        break;

                int w = wcwidth(wc);
                if (w < 0)
                        w = 1;

                wtotal += w;
                p += r;
                len -= r;
        }

        return wtotal;
}

inline static int utf8prev(const char *buf, int index)
{
        int pos = index - 1;
        while (pos > 0 && !utf8lead(buf[pos]))
                pos--;

        return pos;
}

static void history_load(struct termline *line, char *buf)
{
        line->bufptr = buf;
        line->cursor = utf8wtotal(line->bufptr);
        line->maxcol = line->cursor;
        line->len = strlen(line->bufptr);
        line->index = line->len;

        redraw(line);
}

static void history_prev(struct termline *line)
{
        line->hidx--;
        if (line->hidx < 0)
                line->hidx = 0;

        history_load(line, line->snap->lines[line->hidx]);
}

static void history_next(struct termline *line)
{
        line->hidx++;

        if (line->hidx >= (int) line->snap->len) {
                history_load(line, line->saveptr);
                line->hidx = line->snap->len;
        } else {
                history_load(line, line->snap->lines[line->hidx]);
        }
}

static ssize_t put_utf8(struct termline *line, char c)
{
        int len = utf8len(c);

        char tmp[len];
        memset(tmp, 0, len);

        tmp[0] = c;

        for (int i = 1; i < len; i++) {
                if (read(STDIN_FILENO, &c, 1) == -1)
                        return -1;
                tmp[i] = c;
        }

        memmove(&line->bufptr[line->index + len], &line->bufptr[line->index], line->len - line->index);
        memcpy(&line->bufptr[line->index], tmp, len);
        line->len += len;

        int width = utf8width(tmp);
        line->index += len;
        line->cursor += width;
        line->maxcol += width;

        redraw(line);

        return 0;
}

static void cursor_back(struct termline *line)
{
        if (line->index <= 0)
                return;

        int pos = utf8prev(line->bufptr, line->index);
        int len = utf8len(line->bufptr[pos]);
        int width = utf8width(&line->bufptr[pos]);

        line->index -= len;
        line->cursor -= width;

        printf("\x1b[%dG", line->start + line->cursor);
}

static void cursor_fwd(struct termline *line)
{
        if (line->index >= line->len)
                return;

        int len = utf8len(line->bufptr[line->index]);
        int width = utf8width(&line->bufptr[line->index]);

        line->index += len;
        line->cursor += width;

        printf("\x1b[%dG", line->start + line->cursor);
}

static void backspace(struct termline *line)
{
        if (line->index == 0)
                return;

        int pos = utf8prev(line->bufptr, line->index);
        int len = utf8len(line->bufptr[pos]);
        int width = utf8width(&line->bufptr[pos]);

        memmove(&line->bufptr[pos], &line->bufptr[pos + len], line->len - pos);

        line->index -= len;
        line->cursor -= width;
        line->maxcol -= width;
        line->len -= len;

        printf("\x1b[%dG", line->start + line->cursor);
        printf("\x1b[K");
        write(STDOUT_FILENO, &line->bufptr[line->index], line->len - line->index);
        printf("\x1b[%dG", line->start + line->cursor);
}

static void cursor_home(struct termline *line)
{
        line->index = 0;
        line->cursor = 0;
        printf("\x1b[%dG", line->start + line->cursor);
}

static void cursor_end(struct termline *line)
{
        line->index = line->len;
        line->cursor = line->maxcol;
        printf("\x1b[%dG", line->start + line->cursor);
}

static void cursor_delete(struct termline *line)
{
        if (line->index >= line->len)
                return;

        int len = utf8len(line->bufptr[line->index]);
        int width = utf8width(&line->bufptr[line->index]);
        line->maxcol -= width;

        memmove(&line->bufptr[line->index], &line->bufptr[line->index + len], line->len - line->index);
        line->len -= len;

        printf("\x1b[%dG", line->start + line->cursor);
        printf("\x1b[K");
        write(STDOUT_FILENO, &line->bufptr[line->index], line->len - line->index);
        printf("\x1b[%dG", line->start + line->cursor);
}

static void escape(struct termline *line)
{
        int c = getchar();
        if (c != '[')
                return;

        c = getchar();
        switch (c) {
                case 'A': history_prev(line); return;
                case 'B': history_next(line); return;
                case 'D': cursor_back(line); return;
                case 'C': cursor_fwd(line); return;
                case 'H': cursor_home(line); return;
                case 'F': cursor_end(line); return;
                case '1':
                        if (getchar() == '~')
                                cursor_home(line);
                        return;
                case '4':
                        if (getchar() == '~')
                                cursor_end(line);
                        return;
                case '3':
                        if (getchar() == '~')
                                cursor_delete(line);
                        return;
                default:;
        }
}

char *readline(const char *prompt)
{
        raw_on();
        printf("%s", prompt);

        char c;
        struct termline line;
        memset(&line, 0, sizeof(line));
        line.start = (int) strlen(prompt) + 1;
        line.snap = history_copy(hist);
        line.hidx = line.snap->len;

        /* edit_buf 保存当前用户输入最新的一行数据，通过 save 指针保存。
         * line->bufptr 在程序运行的过程中可能会随时指向不同的内存快照 snap（历史记录切换时），
         * 方便用户随时编辑历史行，最终读取输入行不一定在 edit_buf 中，也可能是在 line->snap 中。
         */
        char *edit_buf = malloc(LINE_BUFF_MAX);
        memset(edit_buf, 0, LINE_BUFF_MAX);
        line.bufptr = edit_buf;
        line.saveptr = edit_buf;

        while (1) {
                if (read(STDIN_FILENO, &c, 1) == -1)
                        break;

                if (c == '\n') {
                        write(STDOUT_FILENO, "\n", 1);
                        line.bufptr[line.len] = '\0';
                        break;
                }

                switch (c) {
                        case '\x1b': escape(&line); break;
                        case '\x7f': backspace(&line); break;
                        default:
                                if (put_utf8(&line, c) == -1)
                                        goto out;

                }
        }

out:
        raw_off();

        char *ret = strdup(line.bufptr);
        free(edit_buf);

        /* 清理历史快照内存 */
        history_free(line.snap);

        /* 如果不是空字符串则返回 */
        if (ret[0])
                return ret;

        free(ret);
        return NULL;
}

void add_history(const char *line)
{
        if (!line || !*line)
                return;

        if (hist->len >= hist->max) {
                free(hist->lines[0]);
                memmove(hist->lines, hist->lines + 1, (hist->max - 1) * sizeof(char *));
                strncpy(hist->lines[hist->max - 1], line, strlen(line));
        } else {
                strncpy(hist->lines[hist->len++], line, strlen(line));
        }
}

const char **get_history(size_t *p_size)
{
        *p_size = hist->len;
        return (const char **) hist->lines;
}

void clear()
{
        printf("\x1b[2J\x1b[H");
}