#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "search.h"
#include "regex.h"
#include "file.h"
#include "color.h"
#include "util.h"
#include "oniguruma.h"

static char tbl[AVAILABLE_ENCODING_COUNT][TABLE_SIZE];
static bool tbl_created[AVAILABLE_ENCODING_COUNT] = { 0 };

void generate_bad_character_table(const char *pattern, enum file_type t)
{
    if (tbl_created[t]) {
        return;
    }

    int i, m = strlen(pattern);
    for (i = 0; i < TABLE_SIZE; ++i) {
        tbl[t][i] = m + 1;
    }
    for (i = 0; i < m; ++i) {
        tbl[t][(unsigned char)pattern[i]] = m - i;
    }

    tbl_created[t] = true;
}

/**
 * Format search results. This method does formatting results every line and colorize results.
 */
int format(const char *buf, const match *matches, int match_count, int read_len, const hw_option *op, matched_line_queue *match_lines)
{
    int match_line_count = 0;
    matched_line_queue_node *node;

    for (int i = 0; i < match_count; ) {
        // Find the end of the line.
        int line_end = matches[i].line_end;

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
        strncat(node->line, buf + print_start, line_end - print_start);
        enqueue_matched_line(match_lines, node);

        match_line_count++;
    }

    return match_line_count;
}

/**
 * A fast pattern matching algorithm.
 * This method was proposed by http://www.math.utah.edu/~palais/dnamath/patternmatching.pdf
 */
int ssabs(const unsigned char *buf,
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
                    matches[l].line_end = j + k;
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
                matches[l].line_end = j - 1;
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
int regex(const unsigned char *buf,
          int read_len,
          int line_no_offset,
          const char *pattern,
          match *matches,
          int max_match_size,
          enum file_type t,
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
    regex_t *reg = onig_new_wrap(pattern, t);
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
int search(int fd, const char *pattern, const hw_option *op, enum file_type t, matched_line_queue *match_lines, int *sum_of_actual_match_count)
{
    int n = N, m = strlen(pattern);
    int read_len, actual_match_count, last_line_start;
    int line_no_offset = 1, sum_of_match_count = 0, read_bytes = 0;
    char *buf = (char *)malloc(sizeof(char) * n);

    if (!op->use_regex) {
        generate_bad_character_table(pattern, t);
    }
    *sum_of_actual_match_count = 0;

    // Read every N(65536) bytes until reach to EOF. This method is efficient for memory if file
    // size is very large. And maybe, almost source code files falls within 65536 bytes, so almost
    // files finishes 1 time read.
    while ((read_len = read(fd, buf, n)) > 0) {
        // Check if pointer was reached to the end of the file.
        bool eof = read_len < n;

        int match_size = read_len / m;
        if (match_size > MAX_MATCH_COUNT && !op->no_omit) {
            match_size = MAX_MATCH_COUNT;
        }
        match matches[match_size];

        int match_count;
        if (op->use_regex) {
            // Using a regular expression.
            match_count = regex(
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
        } else {
            // Using SSABS pattern matching algorithm.
            match_count = ssabs(
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

        if (!eof && last_line_start > 0) {
            while (match_count > 0 && matches[match_count - 1].line_no == line_no_offset) {
                match_count--;
            }
        }

        // Format search results.
        // This step will be skiped if `file_with_matches` option (shows only filenames) was passed
        // when the program was launched because the result buffer is not necessary.
        if (!op->file_with_matches) {
            format(buf, matches, match_count, read_len, op, match_lines);
        }

        if (eof) {
            break;
        }

        // If we can't read the contents from the file one time, we should read from the position
        // of the head of the last line in the next iteration because the contents may be broken up.
        // So the file pointer will be seeked, then the next iteration starts from there.
        if (last_line_start > 0) {
            read_bytes += last_line_start;
        } else if (match_count > 0) {
            if (matches[match_count - 1].end >= n - op->pattern_len) {
                read_bytes += n;
            } else {
                read_bytes += n - op->pattern_len;
            }
        } else {
            read_bytes += n;
        }
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
