#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "search.h"
#include "print.h"
#include "regex.h"
#include "hwmalloc.h"
#include "file.h"
#include "fjs.h"
#include "color.h"
#include "util.h"
#include "line_list.h"
#include "oniguruma.h"

#define DOT_LENGTH 4
#define APPEND_DOT(c,t) if(c)strcat((t), OMIT_COLOR);\
                             strcat((t), "....");\
                        if(c)strcat((t), RESET_COLOR)

bool is_word_match(const char *buf, int len, match *m)
{
    return (m->start == 0       || is_word_sp(buf[m->start - 1])) &&
           (m->end   == len - 1 || is_word_sp(buf[m->end      ]));
}

/**
 * Search backward from the end of the `n` bytes pointed to by `buf`. The buffer and target char is
 * compared as unsigned char.
 */
char *reverse_char(const char *buf, char c, size_t n)
{
    if (n == 0) {
        return NULL;
    }

    unsigned char *p = (unsigned char *)buf;
    unsigned char uc = c;
    while (--n != 0) {
        if (buf[n] == uc) return (char *)buf + n;
    }
    return p[n] == uc ? (char *)p : NULL;
}

/**
 * Scan the new line in the buffer from `from to `to`. Scaned new line count will be stored to the
 * `line_count` pointer.
 */
char *scan_newline(char *from, char *to, size_t *line_count, char eol)
{
    const char *start = from;
    while (start <= to && (start = memchr(start, eol, to - start + 1)) != NULL) {
        // Found the new line. The `start` variable points to a new line position, so it is
        // increments in order to search next line.
        start++;

        // Also line count is incriments.
        (*line_count)++;
    }

    return to + 1;
}

char *grow_buf_if_shortage(size_t *cur_buf_size,
                           size_t need_size,
                           int buf_offset,
                           char *copy_buf,
                           char *current_buf)
{
    char *new_buf;
    if (*cur_buf_size < need_size + buf_offset + NMAX) {
        *cur_buf_size += need_size + (NMAX - need_size % NMAX);
        new_buf = (char *)hw_calloc(*cur_buf_size, SIZE_OF_CHAR);
        memcpy(new_buf, copy_buf, need_size);
        tc_free(current_buf);
    } else {
        new_buf = copy_buf;
    }

    return new_buf;
}

int ch(const char *buf, ssize_t search_len, char ch, match *m)
{
    for (size_t i = 0; i < search_len; i++) {
        if (buf[i] == ch) {
            m->start = i;
            m->end   = i + 1;

            if (op.word_regex) {
                if (is_word_match(buf, search_len, m)) {
                    return true;
                }
            } else {
                return true;
            }
        }
    }

    return false;
}

bool search_by(const char *buf,
              size_t search_len,
              const char *pattern,
              int pattern_len,
              enum file_type t,
              match *m,
              int thread_no)
{
    if (op.use_regex) {
        return regex(buf, search_len, pattern, t, m, thread_no);
    } else if (pattern_len == 1) {
        return ch(buf, search_len, pattern[0], m);
    } else {
        return fjs(buf, search_len, pattern, pattern_len, t, m);
    }
}

void format_line_middle(char *line, const char *buf, size_t len, enum append_type type)
{
    if (len == 0) {
        return;
    }

    if (!op.no_omit && len > op.omit_threshold) {
        size_t rest_len;
        switch (type) {
            case AT_FIRST:
                rest_len = op.omit_threshold - DOT_LENGTH;
                APPEND_DOT(op.color, line);
                strncat(line, buf + len - rest_len, rest_len);
                break;
            case AT_MIDDLE:
                rest_len = (op.omit_threshold - DOT_LENGTH) / 2;
                strncat(line, buf, rest_len);
                APPEND_DOT(op.color, line);
                strncat(line, buf + len - rest_len, rest_len);
                break;
            case AT_LAST:
                strncat(line, buf, op.omit_threshold - DOT_LENGTH);
                APPEND_DOT(op.color, line);
                break;
        }
    } else {
        strncat(line, buf, len);
    }
}

/**
 * Search PATTERN from the line and format.
 */
int format_line(const char *line,
                size_t line_len,
                const char *pattern,
                int pattern_len,
                enum file_type t,
                int line_no,
                match *first_match,
                match_line_list *match_lines,
                int thread_no)
{
    // Create new match object default size. Maybe, in the most case, default size is very enough
    // because the PATTERN is appeared in one line only less than 10 count.
    int n = 10;
    match *matches = (match *)hw_malloc(sizeof(match) * n);

    int match_count = 1;
    int offset = first_match->end;
    matches[0] = *first_match;

    // Search the all of PATTERN in the line.
    while (search_by(line + offset, line_len - offset, pattern, pattern_len, t, &matches[match_count], thread_no)) {
        matches[match_count].start += offset;
        matches[match_count].end   += offset;
        offset = matches[match_count].end;

        match_count++;

        // Two times memory will be reallocated if match size is not enough.
        if (n <= match_count) {
            n *= 2;
            matches = (match *)hw_realloc(matches, sizeof(match) * n);
        }
    }

    // Allocate memory for colorized result string.
    int buffer_len = line_len + (op.color_match_len + OMIT_ESCAPE_LEN) * match_count;
    match_line_node *node = (match_line_node *)hw_malloc(sizeof(match_line_node));
    node->line_no = line_no;
    node->context = CONTEXT_NONE;
    node->line = (char *)hw_calloc(buffer_len, SIZE_OF_CHAR);

    const char *start = line;
    int old_end = 0;
    for (int i = 0; i < match_count; i++) {
        /* format_match_string(node->line, start, &matches[i], old_end, i); */
        match m = matches[i];

        // Append prefix string.
        int prefix_len = m.start - old_end;
        format_line_middle(node->line, start, prefix_len, i == 0 ? AT_FIRST : AT_MIDDLE);

        // Append matching word with color.
        strncat_with_color(node->line, start + prefix_len, m.end - m.start, op.color_match);

        start += m.end - old_end;
        old_end = m.end;
    }

    // Append suffix string.
    format_line_middle(node->line, start, line_len - matches[match_count - 1].end, AT_LAST);

    if (match_lines == NULL) {
        print_line(NULL, t, node, 1);
    } else {
        enqueue_match_line(match_lines, node);
    }

    tc_free(matches);

    return match_count;
}

void before_context(const char *buf,
                    const char *line_head,
                    const char *last_match_line_end_pos,
                    int line_no,
                    match_line_list *match_lines,
                    enum file_type t,
                    char eol)
{
    int lim = MAX(op.context, op.before_context);
    const char *lines[lim + 1];
    lines[0] = line_head;

    int before_count = 0;
    for (int i = 0; i < lim; i++) {
        if (lines[i] == buf || last_match_line_end_pos == lines[i]) {
            break;
        }

        const char *p = reverse_char(buf, eol, lines[i] - buf - 1);
        p = p == NULL ? buf : p + 1;

        lines[i + 1] = p;
        before_count++;
    }

    for (int i = before_count; i > 0; i--) {
        int line_len = lines[i - 1] - lines[i] - 1;

        match_line_node *node = (match_line_node *)hw_malloc(sizeof(match_line_node));
        node->line_no = line_no - i;
        node->context = CONTEXT_BEFORE;
        node->line = (char *)hw_calloc(line_len + 1, SIZE_OF_CHAR);

        if (!op.no_omit && line_len > op.omit_threshold) {
            strncat(node->line, lines[i], op.omit_threshold - DOT_LENGTH);
            APPEND_DOT(op.color, node->line);
        } else {
            strncat(node->line, lines[i], line_len);
        }

        if (match_lines == NULL) {
            print_line(NULL, t, node, 1);
        } else {
            enqueue_match_line(match_lines, node);
        }
    }
}

/**
 * Collect after lines from `last_line_end`.
 */
const char *after_context(const char *next_line_head,
                          const char *last_line_end,
                          ssize_t rest_len,
                          int line_no,
                          match_line_list *match_lines,
                          enum file_type t,
                          char eol,
                          int *count)
{
    const char *current = last_line_end;
    int lim = MAX(op.context, op.after_context);
    *count = 0;

    for (int i = 0; i < lim; i++) {
        // Break this loop if the current line reaches to the next line.
        if (next_line_head == current || rest_len <= 0) {
            break;
        }

        const char *after_line = memchr(current, eol, rest_len);
        if (after_line == NULL) {
            after_line = current + rest_len;
        }

        int line_len = after_line - current;

        match_line_node *node = (match_line_node *)hw_malloc(sizeof(match_line_node));
        node->line_no = line_no + i + 1;
        node->context = CONTEXT_AFTER;
        node->line = (char *)hw_calloc(line_len + 1, SIZE_OF_CHAR);

        if (!op.no_omit && line_len > op.omit_threshold) {
            strncat(node->line, current, op.omit_threshold - DOT_LENGTH);
            APPEND_DOT(op.color, node->line);
        } else {
            strncat(node->line, current, line_len);
        }

        if (match_lines == NULL) {
            print_line(NULL, t, node, 1);
        } else {
            enqueue_match_line(match_lines, node);
        }
        (*count)++;

        rest_len -= after_line - current + 1;
        current = ++after_line;
    }

    return current;
}

/**
 * Search the pattern from the buffer as a specified encoding `t`. If matching string was found,
 * results will be added to `match_lines` list. This method do also scanning new lines, and count
 * up it.
 */
int search_buffer(const char *buf,
                  size_t search_len,
                  const char *pattern,
                  int pattern_len,
                  enum file_type t,
                  char eol,
                  size_t *line_count,
                  char **last_new_line_scan_pos,
                  match_line_list *match_lines,
                  int thread_no)
{
    match m;
    const char *p = buf;
    int after_count = 0;
    int match_count = 0;

    // Search the first pattern in the buffer.
    while (search_by(p, search_len, pattern, pattern_len, t, &m, thread_no)) {
        // Search head/end of the line, then calculate line length from them.
        int plen = m.end - m.start;
        size_t rest_len = search_len - m.start - plen + 1;
        const char *line_head = reverse_char(p, eol, m.start);
        char *line_end  = memchr(p + m.start + plen, eol, rest_len);
        line_head = line_head == NULL ? p : line_head + 1;

        // Collect after context.
        const char *last_line_end_by_after = p;
        if (match_count > 0 && (op.after_context > 0 || op.context > 0)) {
            last_line_end_by_after = after_context(line_head, p, search_len, *line_count, match_lines, t, eol, &after_count);
        }

        // Count lines.
        if (op.show_line_number) {
            *last_new_line_scan_pos = scan_newline(*last_new_line_scan_pos, line_end, line_count, eol);
        }

        // Collect before context.
        if (op.before_context > 0 || op.context > 0) {
            before_context(buf, line_head, last_line_end_by_after, *line_count, match_lines, t, eol);
        }

        // Search next pattern in the current line and format them in order to print.
        m.start -= line_head - p;
        m.end    = m.start + plen;
        match_count += format_line(
            line_head,
            line_end - line_head,
            pattern,
            plen,
            t,
            *line_count,
            &m,
            match_lines,
            thread_no
        );

        size_t diff = line_end - p + 1;
        if (search_len < diff) {
            break;
        }
        search_len -= diff;
        p = line_end + 1;
    }

    // Collect last after context. And calculate max line number in this file in order to do
    // padding line number on printing result.
    if (match_count > 0 && search_len > 0 && (op.after_context > 0 || op.context > 0)) {
        after_context(NULL, p, search_len, *line_count, match_lines, t, eol, &after_count);
    }

    if (match_lines != NULL) {
        match_lines->max_line_no = *line_count + after_count;
    }

    return match_count;
}

/**
 * Search the pattern from the file descriptor and add formatted matched lines to the queue if the
 * pattern was matched in the read buffer. This method processes follow steps:
 * 1. The file content will be read to a large buffer at once.
 * 2. Search the pattern from the read buffer.
 * 3. Scan new line count if need.
 *
 * This method returns match count.
 */
int search(int fd,
           const char *pattern,
           int pattern_len,
           enum file_type t,
           match_line_list *match_lines,
           int thread_no)
{
    char eol = '\n';
    size_t line_count = 0;
    size_t read_sum = 0;
    size_t n = NMAX;
    ssize_t read_len;
    int buf_offset = 0;
    int match_count = 0;
    bool do_search = false;
    char *buf = (char *)hw_calloc(n + 1, SIZE_OF_CHAR);
    char *last_new_line_scan_pos = buf;
    char *last_line_end;

    if (!op.use_regex) {
        prepare_fjs(pattern, pattern_len, t);
    }

    while ((read_len = read(fd, buf + buf_offset, NMAX)) > 0) {
        read_sum += read_len;

        // Search end position of the last line in the buffer. We search from the first position
        // and end position of the last line.
        size_t search_len;
        if (read_len < NMAX) {
            last_line_end = buf + read_sum;
            search_len = read_sum;
            buf[read_sum] = eol;
        } else {
            last_line_end = reverse_char(buf + buf_offset, eol, read_len);
            if (last_line_end == NULL) {
                buf = last_new_line_scan_pos = grow_buf_if_shortage(&n, read_sum, buf_offset, buf, buf);
                buf_offset += read_len;
                continue;
            }
            search_len = last_line_end - buf;
        }

        do_search = true;

        // Search the pattern and construct matching results. The results will be stored to list
        // `match_lines`.
        int count = search_buffer(
            buf,
            search_len,
            pattern,
            pattern_len,
            t,
            eol,
            &line_count,
            &last_new_line_scan_pos,
            match_lines,
            thread_no
        );
        match_count += count;

        // Break loop if file pointer is reached to EOF. But if the file descriptor is stdin, we
        // should wait for next input. For example, if hw search from the pipe that is created by
        // `tail -f`, we should continue searching until receive a signal.
        if (fd != STDIN_FILENO && read_len < NMAX) {
            break;
        }

        if (op.show_line_number) {
            last_new_line_scan_pos = scan_newline(last_new_line_scan_pos, last_line_end, &line_count, eol);
        }
        last_line_end++;

        ssize_t rest = read_sum - search_len - 1;
        if (rest >= 0) {
            char *new_buf = grow_buf_if_shortage(&n, rest, 0, last_line_end, buf);
            if (new_buf == last_line_end) {
                new_buf = buf;
                memmove(new_buf, last_line_end, rest);
            }
            buf = last_new_line_scan_pos = new_buf;

            buf_offset = rest;
            read_sum = rest;
        }
    }

    tc_free(buf);
    return match_count;
}
