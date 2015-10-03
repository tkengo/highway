#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "search.h"
#include "regex.h"
#include "file.h"
#include "color.h"
#include "util.h"
#include "oniguruma.h"
#include "string.h"

#define is_utf8_lead_byte(p) (((p) & 0xC0) != 0x80)

static char tbl[AVAILABLE_ENCODING_COUNT][TABLE_SIZE];
static bool tbl_created[AVAILABLE_ENCODING_COUNT] = { 0 };

static int betap[100 + 1];

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

void makebetap(const char *pattern, int m)
{
    const unsigned char *p = (unsigned char *)pattern;
    int i = 0, j = betap[0] = -1;
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
}

void generate_bad_character_table(const char *pattern, enum file_type t)
{
    if (tbl_created[t]) {
        return;
    }

    int i, m = strlen(pattern);
    const unsigned char *p = (unsigned char *)pattern;
    for (i = 0; i < TABLE_SIZE; ++i) {
        tbl[t][i] = m + 1;
    }
    for (i = 0; i < m; ++i) {
        tbl[t][p[i]] = m - i;
    }

    makebetap(pattern, m);
    tbl_created[t] = true;
}

int fjs(const char *buf, int search_len, const char *pattern, int pattern_len, enum file_type t)
{
    const unsigned char *p = (unsigned char *)pattern;
    const unsigned char *x = (unsigned char *)buf;
    int n = search_len, m = pattern_len;

    if (m < 1) {
        return -1;
    }

    int i = 0, j = 0, mp = m - 1, ip = mp;
    while (ip < n) {
        if (j <= 0) {
            while (p[mp] != x[ip]) {
                ip += tbl[t][x[ip + 1]];
                if (ip >= n) return -1;
            }
            j = 0;
            i = ip - mp;
            while (j < mp && x[i] == p[j]) {
                i++; j++;
            }
            if (j == mp) {
                return i - mp;
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
                return i - m;
            }
            j = betap[j];
        }
        ip = i + mp - j;
    }

    return -1;
}

/**
 * Format search results. This method does formatting and colorize for every lines.
 */
int format(const char *buf, int read_len, const match *matches, int match_count, const hw_option *op, matched_line_queue *match_lines)
{
    int match_line_count = 0;
    matched_line_queue_node *node;

    for (int i = 0; i < match_count; ) {
        // Find the end of the line.
        int line_end = matches[i].line_end;
        if (line_end == -1) {
            line_end = read_len - 1;
        }

        // Calculate count of the match elements in one line.
        int current_line_no = matches[i].line_no, j = i;
        while (j < match_count && current_line_no == matches[j].line_no) j++;
        int match_count_in_one_line = j - i;

        // Calculate buffer size printed search result. The size is the sum of line length and
        // escape sequence length used to be coloring result.
        int line_start = matches[i].line_start;
        int line_len   = line_end - line_start + 1;
        int buffer_len = line_len + MATCH_WORD_ESCAPE_LEN * match_count_in_one_line;

        node = (matched_line_queue_node *)malloc(sizeof(matched_line_queue_node));
        node->line_no = current_line_no;
        node->line = (char *)calloc(buffer_len, sizeof(char));

        // If multiple matches in one line, for example:
        //
        // pattern: abc
        // buffer:  xxxabcdefghiabcdef
        //             ^^^      ^^^
        //
        // That pattern will be matched two times in the buffer. In this case we want to build
        // the result as a follow:
        //
        // "xxx(yellow)abc(reset)defghi(yellow)abc(reset)def"
        int print_start = line_start;
        while (i < match_count && current_line_no == matches[i].line_no) {
            int match_start   = matches[i].start;
            int pattern_len   = matches[i].end - match_start;
            int prefix_length = match_start - print_start;

            // Concatenate prefix and pattern string with escape sequence for coloring. But
            // same as line no, if stdout is redirected, then no color.
            strncat(node->line, buf + print_start, prefix_length);
            if (!stdout_redirect_to()) {
                strncat(node->line, MATCH_WORD_COLOR, MATCH_WORD_COLOR_LEN);
            }
            strncat(node->line, buf + print_start + prefix_length, pattern_len);
            if (!stdout_redirect_to()) {
                strncat(node->line, RESET_COLOR, RESET_COLOR_LEN);
            }

            print_start = match_start + pattern_len;
            i++;
        }

        // Concatenate suffix.
        strncat(node->line, buf + print_start, line_end - print_start + 1);
        enqueue_matched_line(match_lines, node);

        match_line_count++;
    }

    return match_line_count;
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
        new_buf = (char *)calloc(*cur_buf_size, sizeof(char));
        memcpy(new_buf, copy_buf, need_size);
        free(current_buf);
    } else {
        new_buf = copy_buf;
    }

    return new_buf;
}

int regex(const char *buf,
          size_t search_len,
          const char *pattern,
          int pattern_len,
          enum file_type t,
          const hw_option *op)
{
    OnigRegion *region = onig_region_new();

    // Get the compiled regular expression. Actually, onig_new method is not safety multiple-thread,
    // but this wrapper method is implemented in thread safety.
    regex_t *reg = onig_new_wrap(pattern, t, op->ignore_case);
    if (reg == NULL) {
        return -1;
    }

    // Search every lines using compiled regular expression above.
    int pos = -1;
    const unsigned char *p     = (unsigned char *)buf,
                        *start = p,
                        *end   = start + search_len,
                        *range = end;
    if (onig_search(reg, p, end, start, range, region, ONIG_OPTION_NONE) >= 0) {
        /* matches[match_count].start      = region->beg[0]; */
        /* matches[match_count].end        = region->end[0]; */
        /* matches[match_count].line_no    = line_no; */
        /* matches[match_count].line_end   = -1; */
        pos = region->beg[0];
    }

    onig_region_free(region, 1);
    return pos;
}

/**
 * A fast pattern matching algorithm.
 * This method was proposed by http://www.math.utah.edu/~palais/dnamath/patternmatching.pdf
 *
 * This method search the pattern from the position of the `buf` to `buf` + `buf_len` in file type
 * `t`. Return matched position if the pattern was matched, otherwise -1.
 */
int ssabs(const char *buf,
          int search_len,
          const char *pattern,
          int pattern_len,
          enum file_type t)
{
    const unsigned char *ubuf = (unsigned char *)buf;
    const unsigned char *up   = (unsigned char *)pattern;
    int j = 0, m = pattern_len;
    unsigned char firstCh = pattern[0];
    unsigned char lastCh  = pattern[m - 1];

    while (j <= search_len - m) {
        if (lastCh == ubuf[j + m - 1] && firstCh == ubuf[j]) {
            for (int i = m - 2; i >= 0 && up[i] == ubuf[j + i]; --i) {
                if (i <= 0) {
                    // Pattern matched.
                    return j;
                }
            }
        }
        j += tbl[t][ubuf[j + m]];
    }

    return -1;
}

int search_by(const char *buf,
              size_t search_len,
              const char *pattern,
              int pattern_len,
              enum file_type t,
              const hw_option *op)
{
    if (op->use_regex) {
        return regex(buf, search_len, pattern, pattern_len, t, op);
    } else {
        /* return ssabs(buf, search_len, pattern, pattern_len, t); */
        return fjs(buf, search_len, pattern, pattern_len, t);
    }
}

int format_line(const char *line,
                int line_len,
                const char *pattern,
                int pattern_len,
                enum file_type t,
                int line_no,
                match *matches,
                int match_start,
                const hw_option *op,
                matched_line_queue *match_line)
{
    int pos = 0, match_count = match_start;
    int offset = matches[match_start - 1].end;

    while ((pos = search_by(line + offset, line_len - offset, pattern, pattern_len, t, op)) != -1) {
        pos += offset;

        int end = pos + pattern_len;
        matches[match_count].start = pos;
        matches[match_count].end   = end;
        match_count++;

        offset = end;
    }

    int buffer_len = line_len + MATCH_WORD_ESCAPE_LEN * match_count;
    matched_line_queue_node *node = (matched_line_queue_node *)malloc(sizeof(matched_line_queue_node));
    node->line_no = line_no;
    node->line = (char *)calloc(buffer_len, sizeof(char));

    const char *s = line;
    int old_end = 0;
    for (int i = 0; i < match_count; i++) {
        int prefix_len = matches[i].start - old_end;

        strncat(node->line, s, prefix_len);
        strcat (node->line, MATCH_WORD_COLOR);
        strncat(node->line, s + prefix_len, pattern_len);
        strcat (node->line, RESET_COLOR);

        s += prefix_len + pattern_len;
        old_end = matches[i].end;
    }

    int last_end = matches[match_count - 1].end;
    strncat(node->line, s, line_len - last_end);
    enqueue_matched_line(match_line, node);

    return match_count;
}

/**
 * Search the pattern from the file descriptor and add formatted matched lines to the queue if the
 * pattern was matched on the read buffer.
 */
int search(int fd,
           const char *pattern,
           enum file_type t,
           const hw_option *op,
           matched_line_queue *match_line)
{
    size_t line_count = 0;
    size_t read_sum = 0;
    size_t n = N;
    size_t read_len;
    int pos;
    int pattern_len = op->pattern_len;
    int buf_offset = 0;
    int match_count = 0;
    char *buf = (char *)calloc(n, sizeof(char));
    char *last_new_line_scan_pos = buf;

    if (!op->use_regex) {
        generate_bad_character_table(pattern, t);
    }

    while ((read_len = read(fd, buf + buf_offset, N)) > 0) {
        read_sum += read_len;

        // Search the end of the position of the last line.
        char *last_line_end = reverse_char(buf + buf_offset, '\n', read_len);
        if (last_line_end == NULL) {
            buf = last_new_line_scan_pos = grow_buf_if_shortage(&n, read_sum, buf_offset, buf, buf);
            buf_offset += read_len;
            continue;
        }

        size_t search_len = last_line_end == NULL ? read_sum : last_line_end - buf;
        size_t org_search_len = search_len;
        char *p = buf;
        while ((pos = search_by(p, search_len, pattern, pattern_len, t, op)) != -1) {
            // Searching head/end of the line, then calculate line length using them.
            char *line_head = reverse_char(p, '\n', pos);
            char *line_end  = memchr(p + pos + pattern_len, '\n', search_len - pos - pattern_len + 1);
            line_head = line_head == NULL ? p : line_head + 1;
            int line_len = line_end - line_head;

            // Count lines.
            last_new_line_scan_pos = scan_newline(last_new_line_scan_pos, line_end, &line_count);

            match matches[MIN(line_len / pattern_len, MAX_MATCH_COUNT)];
            matches[0].start = pos - (line_head - p);
            matches[0].end   = matches[0].start + pattern_len;
            match_count += format_line(line_head, line_len, pattern, pattern_len, t, line_count, matches, 1, op, match_line);

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

/**
 * A fast pattern matching algorithm.
 * This method was proposed by http://www.math.utah.edu/~palais/dnamath/patternmatching.pdf
 */
int ssabs_(const unsigned char *buf,
          int buf_len,
          int line_no_offset,
          const char *pattern,
          match *matches,
          int max_match_size,
          enum file_type t,
          int *actual_match_count,
          int *last_line_start,
          int *last_line_no)
{
    int i, j = 0, m = strlen(pattern);
    int match_count = 0;
    int line_no = line_no_offset;
    int line_start = 0;
    unsigned char *p = (unsigned char *)pattern;
    unsigned char firstCh = p[0];
    unsigned char lastCh  = p[m - 1];
    *actual_match_count = 0;

    while (j <= buf_len - m) {
        if (lastCh == buf[j + m - 1] && firstCh == buf[j]) {
            for (i = m - 2; i >= 0 && p[i] == buf[j + i]; --i) {
                if (i <= 0) {
                    // Pattern matched.
                    if (max_match_size > match_count) {
                        matches[match_count].start      = j;
                        matches[match_count].end        = j + m;
                        matches[match_count].line_no    = line_no;
                        matches[match_count].line_start = line_start;
                        matches[match_count].line_end   = -1;
                        match_count++;
                    }

                    (*actual_match_count)++;
                }
            }
        }
        int skip = tbl[t][buf[j + m]];
        for (int k = 0; k < skip; k++) {
            unsigned char t = buf[j + k];
            if (t == 0x0A || t == 0x0D) {
                int l = match_count - 1;
                while (l >= 0 && matches[l].line_end == -1 && line_no == matches[l].line_no) {
                    matches[l].line_end = j + k - 1;
                    l--;
                }

                line_no++;
                line_start = j + k + 1;
            }
        }
        j += skip;
    }

    while (j < buf_len) {
        unsigned char t = buf[j++];
        if (t == 0x0A || t == 0x0D) {
            int l = match_count - 1;
            while (l >= 0 && matches[l].line_end == -1 && line_no == matches[l].line_no) {
                matches[l].line_end = j - 2;
                l--;
            }

            line_no++;
            line_start = j;
        }
    }

    *last_line_start = line_start;
    *last_line_no    = line_no;

    return match_count;
}

/**
 * Search the pattern as a regular expression.
 */
int regex_(const unsigned char *buf,
          int read_len,
          int line_no_offset,
          const char *pattern,
          match *matches,
          int max_match_size,
          enum file_type t,
          bool ignore_case,
          int *actual_match_count,
          int *last_line_start,
          int *last_line_no)
{
    OnigRegion *region = onig_region_new();
    int match_count = 0;
    int i = 0, line_start = 0, line_no = line_no_offset;
    *actual_match_count = 0;

    // Get the compiled regular expression. Actually, onig_new method is not safety multiple-thread,
    // but this wrapper method of the onig_new is implemented in thread safety.
    regex_t *reg = onig_new_wrap(pattern, t, ignore_case);
    if (reg == NULL) {
        return 0;
    }

    // Search every lines using compiled regular expression above.
    while (i < read_len) {
        if (buf[i] == 0x0A || buf[i] == 0x0D) {
            // Skip if current line has no contents.
            if (i != line_start) {
                int pos = 0;
                while (1) {
                    const unsigned char *start = buf + line_start + pos,
                                        *end   = buf + i,
                                        *range = end;
                    int r = onig_search(reg, buf, end, start, range, region, ONIG_OPTION_NONE);
                    if (r >= 0) {
                        if (max_match_size > match_count) {
                            matches[match_count].start      = region->beg[0];
                            matches[match_count].end        = region->end[0];
                            matches[match_count].line_no    = line_no;
                            matches[match_count].line_start = line_start;
                            matches[match_count].line_end   = -1;
                            match_count++;
                        }
                        (*actual_match_count)++;

                        if (i <= region->end[0]) {
                            break;
                        }
                        pos += region->end[0] - line_start;
                    } else {
                        break;
                    }
                }
            }

            int l = match_count - 1;
            while (l >= 0 && matches[l].line_end == -1 && line_no == matches[l].line_no) {
                matches[l].line_end = i - 1;
                l--;
            }

            line_start = i + 1;
            line_no++;
        }

        i++;
    }

    onig_region_free(region, 1);

    *last_line_start = line_start;
    *last_line_no    = line_no;

    return match_count;
}

/**
 * Search the pattern from the file descriptor and add formatted matched lines to the queue if the
 * pattern was matched on the read buffer.
 *
 * @param fd File descriptor. The search buffer was read from this fd.
 * @param op The pattern string is included this struct.
 * @return The count of the matched lines.
 */
int search_(int fd, const char *pattern, const hw_option *op, enum file_type t, matched_line_queue *match_lines, int *sum_of_actual_match_count)
{
    int m = strlen(pattern);
    int read_len, actual_match_count, last_line_start;
    int line_no_offset = 1, sum_of_match_count = 0, read_bytes = 0;
    char *buf = (char *)malloc(sizeof(char) * N);

    if (!op->use_regex) {
        generate_bad_character_table(pattern, t);
    }
    *sum_of_actual_match_count = 0;

    // Read every N(65536) bytes until reach to EOF. This method is efficient for memory if file
    // size is very large. And maybe, almost source code files falls within 65536 bytes, so almost
    // files finishes 1 time read.
    while ((read_len = read(fd, buf, N)) > 0) {
        // Check if pointer was reached to the end of the file.
        bool eof = read_len < N;

        int match_size = read_len / m;
        if (match_size > MAX_MATCH_COUNT && !op->no_omit) {
            match_size = MAX_MATCH_COUNT;
        }
        match matches[match_size];

        int match_count;
        if (op->use_regex) {
            // Using a regular expression.
            match_count = regex_(
                (unsigned char *)buf,
                read_len,
                line_no_offset,
                pattern,
                matches,
                match_size,
                t,
                op->ignore_case,
                &actual_match_count,
                &last_line_start,
                &line_no_offset
            );
        } else {
            // Using SSABS pattern matching algorithm.
            match_count = ssabs_(
                (unsigned char *)buf,
                read_len,
                line_no_offset,
                pattern,
                matches,
                match_size,
                t,
                &actual_match_count,
                &last_line_start,
                &line_no_offset
            );
        }

         sum_of_match_count        += match_count;
        *sum_of_actual_match_count += actual_match_count;

        if (!eof) {
            if (last_line_start > 0) {
                while (match_count > 0 && matches[match_count - 1].line_no == line_no_offset) {
                    match_count--;
                }
            } else {
                if (match_count == 0 || matches[match_count - 1].end < read_len) {
                    read_len -= op->use_regex ? 100 : op->pattern_len;
                }
                while (!is_utf8_lead_byte(buf[read_len])) {
                    read_len--;
                }
            }
        }

        // Format search results.
        // This step will be skiped if `file_with_matches` option (shows only filenames) was passed
        // when the program was launched because the result buffer is not necessary.
        if (!op->file_with_matches && match_count > 0) {
            format(buf, read_len, matches, match_count, op, match_lines);
        }

        if (eof) {
            break;
        }

        // If we can't read the contents from the file one time, we should seek the file pointer
        // because the contents read from fd may be broken up. So the file pointer will be seeked,
        // then the next iteration starts from there.
        read_bytes += last_line_start > 0 ? last_line_start : read_len;
        lseek(fd, read_bytes, SEEK_SET);
    }

    free(buf);
    return sum_of_match_count;
}

int search_stream(const char *pattern, const hw_option *op)
{
    char buf[N], line[N], filename[1024];
    bool is_print_filename = false;
    int m = strlen(pattern), match_file_count = 0;
    while (fgets(line, N, stdin) != NULL) {
        if (strncmp(line, FILENAME_COLOR, FILENAME_COLOR_LEN) == 0) {
            is_print_filename = false;
            strcpy(filename, line);
            continue;
        }
        if (line[0] == 0x0A || line[0] == 0x0D) {
            continue;
        }

        memset(buf, 0, N);

        char *current = index(line, ':');
        strncpy(buf, line, current - line);
        bool matched = false;
        while (1) {
            char *index = strstr(current + 1, pattern);
            if (index != NULL) {
                strncat(buf, current, index - current);
                strncat(buf, MATCH_WORD_COLOR2, MATCH_WORD_COLOR2_LEN);
                strncat(buf, index, m);
                strncat(buf, RESET_COLOR, RESET_COLOR_LEN);
                current = index + m;
                matched = true;
            } else {
                strcat(buf, current);
                break;
            }
        }

        if (matched) {
            if (!is_print_filename) {
                if (match_file_count > 0) {
                    printf("\n");
                }
                match_file_count++;
                printf("%s", filename);
                is_print_filename = true;
            }
            printf("%s", buf);
        }
    }
    printf("\n");
    return 0;
}
