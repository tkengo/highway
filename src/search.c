#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "search.h"
#include "file.h"
#include "color.h"

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
 * A fast pattern matching algorithm.
 * This method was proposed by http://www.math.utah.edu/~palais/dnamath/patternmatching.pdf
 */
int ssabs(const unsigned char *buf, int buf_len, int line_no_offset, const char *pattern, match *matches, int max_match, enum file_type t, int *last_line_start, int *last_line_no)
{
    int i, j = 0, m = strlen(pattern);
    int match_count = 0;
    int line_no = line_no_offset;
    int line_start = 0;
    unsigned char *p = (unsigned char *)pattern;
    unsigned char firstCh = p[0];
    unsigned char lastCh  = p[m - 1];

    while (j <= buf_len - m) {
        if (lastCh == buf[j + m - 1] && firstCh == buf[j]) {
            for (i = m - 2; i >= 0 && p[i] == buf[j + i]; --i) {
                if (i <= 0) {
                    // matched
                    matches[match_count].start = j;
                    matches[match_count].line_no = line_no;
                    matches[match_count].line_start = line_start;

                    if (max_match == ++match_count) {
                        return match_count;
                    }
                }
            }
        }
        int skip = tbl[t][buf[j + m]];
        for (int k = 0; k < skip; k++) {
            unsigned char t = buf[j + k];
            if (t == 0x0A || t == 0x0D) {
                line_no++;
                line_start = j + k + 1;
            }
        }
        j += skip;
    }

    while (j < buf_len) {
        unsigned char t = buf[j++];
        if (t == 0x0A || t == 0x0D) {
            line_no++;
            line_start = j;
        }
    }

    *last_line_start = line_start;
    *last_line_no    = line_no;

    return match_count;
}

int format(const char *buf, const match *matches, int match_count, int read_len, const char *pattern, const hw_option *op, matched_line_queue *match_lines)
{
    int match_line_count = 0;
    int m = strlen(pattern);

    for (int i = 0; i < match_count; ) {
        // Find the end of the line.
        int line_end = matches[i].start;
        while (line_end < read_len && buf[line_end] != 0x0A && buf[line_end] != 0x0D) line_end++;

        // Calculate buffer size printed search result. The size is the sum of line length and
        // escape sequence length in order to be coloring result.
        int line_start = matches[i].line_start;
        int line_len   = line_end - line_start + 1;
        int buffer_len = line_len + LINE_NO_ESCAPE_LEN + 10 + MATCH_WORD_ESCAPE_LEN * (line_len / m);

        // Generate new node represented matched line.
        matched_line_queue_node *node = (matched_line_queue_node *)malloc(sizeof(matched_line_queue_node));

        // Append line no. It has yellow color.
        // But this step will be skiped if `file_with_matches` option (shows only filenames) was
        // passed when the program was launched because the result buffer is not necessary.
        int current_line_no = matches[i].line_no;
        if (!op->file_with_matches) {
            node->line = (char *)malloc(sizeof(char) * buffer_len);
            sprintf(node->line, "%s%d%s:", LINE_NO_COLOR, current_line_no, RESET_COLOR);
        }

        // If multiple matches in one line, for example:
        //
        // pattern: abc
        // buffer:  xxx abc def ghi abc def
        //              ^^^         ^^^
        //
        // That pattern will be matched two times in the buffer. In this case we want to build
        // the result as a follow:
        //
        // "xxx (yellow)abc(reset) def ghi (yellow)abc(reset) def"
        int print_start = line_start;
        while (i < match_count && current_line_no == matches[i].line_no) {
            // Skip this step if `file_with_matches` option (shows only filenames) is available
            // because the result buffer is not necessary.
            if (!op->file_with_matches) {
                int match_start   = matches[i].start;
                int prefix_length = match_start - print_start + 1;

                char *prefix = (char *)malloc(sizeof(char) * prefix_length);
                strlcpy(prefix, buf + print_start, prefix_length);
                sprintf(node->line + strlen(node->line), "%s%s%s%s", prefix, MATCH_WORD_COLOR, pattern, RESET_COLOR);
                free(prefix);

                print_start = match_start + m;
            }

            i++;
        }

        // Skip this step if `file_with_matches` option (shows only filenames) is available
        // because the result buffer is not necessary.
        if (!op->file_with_matches) {
            int suffix_length = line_end - print_start + 1;
            char *suffix = (char *)malloc(sizeof(char) * suffix_length);
            strlcpy(suffix, buf + print_start, suffix_length);
            sprintf(node->line + strlen(node->line), "%s", suffix);
            free(suffix);

            enqueue_matched_line(match_lines, node);
        }

        match_line_count++;
    }

    return match_line_count;
}

/**
 * Search the pattern from the file descriptor and add formatted matched lines to the queue if the
 * pattern was matched on the read buffer.
 *
 * @param fd File descriptor. The search buffer was read from this fd.
 * @param op The pattern string is included this struct.
 * @return The count of the matched lines.
 */
int search(int fd, const char *pattern, const hw_option *op, enum file_type t, matched_line_queue *match_lines)
{
    int n = N;
    int read_len, last_line_start, line_no_offset = 1, match_line_count = 0, read_bytes = 0;
    char *buf = (char *)malloc(sizeof(char) * n);

    generate_bad_character_table(pattern, t);

    // Read every N(65536) bytes until reach to EOF. This method is efficient for memory if file
    // size is very large. And maybe, almost source code files falls within 65536 bytes, so almost
    // files finishes 1 time read.
    while ((read_len = read(fd, buf, n)) > 0) {
        // Check if pointer was reached to the end of the file.
        bool eof = read_len < n;

        // Using SSABS pattern matching algorithm.
        match matches[MAX_MATCH_COUNT];
        int match_count = ssabs(
            (unsigned char *)buf,
            read_len,
            line_no_offset,
            pattern,
            matches,
            MAX_MATCH_COUNT,
            t,
            &last_line_start,
            &line_no_offset
        );

        if (!eof) {
            // If there is too long line over 65536 bytes, reallocate the twice memory for the
            // current buffer, then search again.
            if (last_line_start == 0) {
                n *= 2;
                buf = (char *)realloc(buf, sizeof(char) * n);
                continue;
            }

            while (match_count > 0 && matches[match_count - 1].line_no == line_no_offset) {
                match_count--;
            }
        }

        // Format search results.
        match_line_count += format(buf, matches, match_count, read_len, pattern, op, match_lines);

        if (eof) {
            break;
        }

        // If we can't read the contents from the file one time, we should read from the position
        // of the head of the last line in the next iteration because the contents may break up.
        // So the file pointer will be seeked to it, then the next iteration starts from there.
        read_bytes += last_line_start;
        lseek(fd, read_bytes, SEEK_SET);
    }

    free(buf);
    return match_line_count;
}
