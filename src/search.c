#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "search.h"
#include "color.h"

static char tbl[TABLE_SIZE];

void generate_bad_character_table(char *pattern)
{
    int i, m = strlen(pattern);
    for (i = 0; i < TABLE_SIZE; ++i) {
        tbl[i] = m + 1;
    }
    for (i = 0; i < m; ++i) {
        tbl[(unsigned char)pattern[i]] = m - i;
    }
}

/**
 * Search the pattern from the file descriptor. Add match lines to the queue if the pattern was
 * matched on the read buffer.
 *
 * @param fd File descriptor. The search buffer was read from this fd.
 * @param op The pattern string is included this struct.
 * @return The count of the matched lines.
 */
int search(int fd, hw_option *op, matched_line_queue *match_lines)
{
    int read_len, match_line_count = 0;
    char *buf = (char *)malloc(sizeof(char) * N);

    // Read every N(65535) bytes until reach to EOF. This method is efficient for memory if file
    // size is very large. And maybe, almost source code files falls within 65535 bytes, so almost
    // files finishes 1 time read.
    while ((read_len = read(fd, buf, N)) > 0) {
        // Using SSABS pattern matching algorithm.
        match matches[MAX_MATCH_COUNT];
        int matches_count = ssabs((unsigned char *)buf, read_len, op->pattern, matches, MAX_MATCH_COUNT);

        // Format result.
        for (int i = 0; i < matches_count; ) {
            // Find the end of the line.
            int line_end = matches[i].start;
            while (buf[line_end] != 0x0A && buf[line_end] != 0x0D) line_end++;

            // Calculate buffer size printed search result. The size is the sum of line length and
            // escape sequence length in order to be coloring result.
            int line_start = matches[i].line_start;
            int line_len   = line_end - line_start + 1;
            int buffer_len = line_len + LINE_NO_ESCAPE_LEN + 10 + MATCH_WORD_ESCAPE_LEN * (line_len / op->pattern_len);

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
            while (i < matches_count && current_line_no == matches[i].line_no) {
                // Skip this step if `file_with_matches` option (shows only filenames) is available
                // because the result buffer is not necessary.
                if (!op->file_with_matches) {
                    int match_start   = matches[i].start;
                    int prefix_length = match_start - print_start + 1;

                    char *prefix = (char *)malloc(sizeof(char) * prefix_length);
                    strlcpy(prefix, buf + print_start, prefix_length);
                    sprintf(node->line + strlen(node->line), "%s%s%s%s", prefix, MATCH_WORD_COLOR, op->pattern, RESET_COLOR);
                    free(prefix);

                    print_start = match_start + op->pattern_len;
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

        if (read_len < N) {
            break;
        }
    }

    free(buf);
    return match_line_count;
}

int ssabs(const unsigned char *buf, int buf_len, const char *pattern, match *matches, int max_match)
{
    int i, j = 0, m = strlen(pattern);

    int match_count = 0;
    int line_no = 1;
    int line_start = 0;
    unsigned char firstCh = pattern[0];
    unsigned char lastCh  = pattern[m - 1];
    while (j <= buf_len - m) {
        if (lastCh == buf[j + m - 1] && firstCh == buf[j]) {
            for (i = m - 2; i >= 0 && (unsigned char)pattern[i] == buf[j + i]; --i) {
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
        int skip = tbl[buf[j + m]];
        for (int k = 0; k < skip; k++) {
            int t = buf[j + k];
            if (t == 0x0A || t == 0x0D) {
                line_no++;
                line_start = j + k + 1;
            }
        }
        j += skip;
    }

    return match_count;
}
