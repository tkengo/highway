#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gperftools/tcmalloc.h>
#include "search.h"
#include "regex.h"
#include "file.h"
#include "color.h"
#include "util.h"
#include "oniguruma.h"

#define is_utf8_lead_byte(p) (((p) & 0xC0) != 0x80)
#define APPEND_DOT(t) strcat((t), OMIT_COLOR);\
                      strcat((t), "....");\
                      strcat((t), RESET_COLOR)

static char tbl[AVAILABLE_ENCODING_COUNT][BAD_CHARACTER_TABLE_SIZE];
static bool tbl_created[AVAILABLE_ENCODING_COUNT] = { 0 };

static int *gbetap[AVAILABLE_ENCODING_COUNT] = { 0 };

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

void prepare_fjs(const char *pattern, int pattern_len, enum file_type t)
{
    if (tbl_created[t]) {
        return;
    }

    // Generate bad-character table.
    int i, j, m = strlen(pattern);
    const unsigned char *p = (unsigned char *)pattern;
    for (i = 0; i < BAD_CHARACTER_TABLE_SIZE; ++i) {
        tbl[t][i] = m + 1;
    }
    for (i = 0; i < m; ++i) {
        tbl[t][p[i]] = m - i;
    }

    // Generate betap.
    int *betap = gbetap[t] = (int *)tc_malloc(sizeof(int) * (pattern_len + 1));
    i = 0;
    j = betap[0] = -1;
    while (i < m) {
        while (j > -1 && p[i] != p[j]) {
            j = betap[j];
        }
        if (p[++i] == p[++j]) {
            betap[i] = betap[j];
        } else {
            betap[i] = j;
        }
    }

    tbl_created[t] = true;
}

void free_fjs()
{
    for (int i = 0; i < AVAILABLE_ENCODING_COUNT; i++) {
        if (gbetap[i] != NULL) {
            free(gbetap[i]);
        }
    }
}

bool fjs(const char *buf, int search_len, const char *pattern, int pattern_len, enum file_type t, match *mt)
{
    const unsigned char *p = (unsigned char *)pattern;
    const unsigned char *x = (unsigned char *)buf;
    int n = search_len, m = pattern_len;

    if (m < 1) {
        return false;
    }

    int *betap = gbetap[t];
    int i = 0, j = 0, mp = m - 1, ip = mp;
    while (ip < n) {
        if (j <= 0) {
            while (p[mp] != x[ip]) {
                ip += tbl[t][x[ip + 1]];
                if (ip >= n) return false;
            }
            j = 0;
            i = ip - mp;
            while (j < mp && x[i] == p[j]) {
                i++; j++;
            }
            if (j == mp) {
                mt->start = i - mp;
                mt->end   = mt->start + m;
                return true;
            }
            if (j <= 0) {
                i++;
            } else {
                j = betap[j];
            }
        } else {
            while (j < m && x[i] == p[j]) {
                i++; j++;
            }
            if (j == m) {
                mt->start = i - m;
                mt->end   = mt->start + m;
                return false;
            }
            j = betap[j];
        }
        ip = i + mp - j;
    }

    return false;
}

char *scan_newline(char *from, char *to, size_t *line_count)
{
    const char *start = from;
    while (start <= to && (start = memchr(start, '\n', to - start + 1)) != NULL) {
        start++;
        (*line_count)++;
    }
    from = to + 1;
    return from;
}

char *grow_buf_if_shortage(size_t *cur_buf_size,
                           size_t need_size,
                           int buf_offset,
                           char *copy_buf,
                           char *current_buf)
{
    char *new_buf;
    if (*cur_buf_size < need_size + buf_offset + N) {
        *cur_buf_size += N;
        new_buf = (char *)tc_calloc(*cur_buf_size, sizeof(char));
        memcpy(new_buf, copy_buf, need_size);
        free(current_buf);
    } else {
        new_buf = copy_buf;
    }

    return new_buf;
}

bool regex(const char *buf,
          size_t search_len,
          const char *pattern,
          enum file_type t,
          match *m)
{
    OnigRegion *region = onig_region_new();

    // Get the compiled regular expression. Actually, onig_new method is not safety multiple-thread,
    // but this wrapper method is implemented in thread safety.
    regex_t *reg = onig_new_wrap(pattern, t, op.ignore_case);
    if (reg == NULL) {
        return false;
    }

    bool res = false;
    const unsigned char *p     = (unsigned char *)buf,
                        *start = p,
                        *end   = start + search_len,
                        *range = end;
    if (onig_search(reg, p, end, start, range, region, ONIG_OPTION_NONE) >= 0) {
        m->start = region->beg[0];
        m->end   = region->end[0];
        res = true;
    }

    onig_region_free(region, 1);
    return res;
}

int search_by(const char *buf,
              size_t search_len,
              const char *pattern,
              int pattern_len,
              enum file_type t,
              match *m)
{
    if (op.use_regex) {
        return regex(buf, search_len, pattern, t, m);
    } else {
        return fjs(buf, search_len, pattern, pattern_len, t, m);
    }
}

int format_line(const char *line,
                int line_len,
                const char *pattern,
                int pattern_len,
                enum file_type t,
                int line_no,
                match *first_match,
                matched_line_queue *match_line)
{
    int n = 10;
    int pos = 0, match_count = 1;
    int offset = first_match->end;
    match *matches = (match *)tc_malloc(sizeof(match) * n);
    matches[0] = *first_match;

    while (search_by(line + offset, line_len - offset, pattern, pattern_len, t, &matches[match_count])) {
        if (n <= match_count) {
            n *= 2;
            matches = (match *)realloc(matches, sizeof(match) * n);
        }

        matches[match_count].start += offset;
        matches[match_count].end   += offset;
        offset = matches[match_count].end;

        match_count++;
    }

    int buffer_len = line_len + (MATCH_WORD_ESCAPE_LEN + OMIT_ESCAPE_LEN) * match_count;
    matched_line_queue_node *node = (matched_line_queue_node *)tc_malloc(sizeof(matched_line_queue_node));
    node->line_no = line_no;
    node->line = (char *)tc_calloc(buffer_len, sizeof(char));

    const char *s = line;
    int old_end = 0;
    int sum = 0;
    for (int i = 0; i < match_count; i++) {
        int prefix_len = matches[i].start - old_end;
        int plen = matches[i].end - matches[i].start;
        sum += prefix_len + plen;

        if (!IS_STDOUT_REDIRECT && matches[i].start - old_end > op.omit_threshold) {
            if (i == 0) {
                int rest_len = op.omit_threshold - 4;
                APPEND_DOT(node->line);
                strncat(node->line, s + prefix_len - rest_len, rest_len);
            } else {
                int rest_len = op.omit_threshold / 2 - 2;
                strncat(node->line, s, rest_len);
                APPEND_DOT(node->line);
                strncat(node->line, s + prefix_len - rest_len, rest_len);
            }
        } else {
            strncat(node->line, s, prefix_len);
        }

        if (!IS_STDOUT_REDIRECT) {
            strcat(node->line, MATCH_WORD_COLOR);
        }

        strncat(node->line, s + prefix_len, plen);

        if (!IS_STDOUT_REDIRECT) {
            strcat(node->line, RESET_COLOR);
        }

        s += prefix_len + plen;
        old_end = matches[i].end;
    }

    int last_end = matches[match_count - 1].end;
    int suffix_len = line_len - last_end;
    if (suffix_len > op.omit_threshold) {
        strncat(node->line, s, op.omit_threshold - 4);
        APPEND_DOT(node->line);
    } else {
        strncat(node->line, s, line_len - last_end);
    }
    enqueue_matched_line(match_line, node);

    return match_count;
}

/**
 * Search the pattern from the file descriptor and add formatted matched lines to the queue if the
 * pattern was matched on the read buffer.
 */
int search(int fd,
           const char *pattern,
           int pattern_len,
           enum file_type t,
           matched_line_queue *match_line)
{
    size_t line_count = 0;
    size_t read_sum = 0;
    size_t n = N;
    size_t read_len;
    int buf_offset = 0;
    int match_count = 0;
    char *buf = (char *)tc_calloc(n, sizeof(char));
    char *last_new_line_scan_pos = buf;
    match m;

    if (!op.use_regex) {
        prepare_fjs(pattern, pattern_len, t);
    }

    while ((read_len = read(fd, buf + buf_offset, N)) > 0) {
        read_sum += read_len;

        // Search end of position of the last line in the buffer.
        char *last_line_end = reverse_char(buf + buf_offset, '\n', read_len);
        if (last_line_end == NULL) {
            buf = last_new_line_scan_pos = grow_buf_if_shortage(&n, read_sum, buf_offset, buf, buf);
            buf_offset += read_len;
            continue;
        }

        size_t search_len = last_line_end == NULL ? read_sum : last_line_end - buf;
        size_t org_search_len = search_len;
        char *p = buf;

        // Search the first pattern in the buffer.
        while (search_by(p, search_len, pattern, pattern_len, t, &m)) {
            // Search head/end of the line, then calculate line length by using them.
            int plen = m.end - m.start;
            char *line_head = reverse_char(p, '\n', m.start);
            char *line_end  = memchr(p + m.start + plen, '\n', search_len - m.start - plen + 1);
            line_head = line_head == NULL ? p : line_head + 1;
            int line_len = line_end - line_head;

            // Count lines.
            last_new_line_scan_pos = scan_newline(last_new_line_scan_pos, line_end, &line_count);

            m.start -= line_head - p;
            m.end    = m.start + plen;
            match_count += format_line(line_head, line_len, pattern, plen, t, line_count, &m, match_line);

            search_len -= line_end - p + 1;
            p = line_end + 1;
        }

        last_new_line_scan_pos = scan_newline(last_new_line_scan_pos, last_line_end, &line_count);

        if (read_len < N) {
            break;
        }

        size_t rest = read_sum - org_search_len - 1;

        last_line_end++;
        char *new_buf = grow_buf_if_shortage(&n, rest, 0, last_line_end, buf);
        if (new_buf == last_line_end) {
            new_buf = buf;
            memmove(new_buf, last_line_end, rest);
        }
        buf = last_new_line_scan_pos = new_buf;

        buf_offset = rest;
        read_sum = rest;
    }

    free(buf);
    return match_count;
}
