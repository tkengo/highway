#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "search.h"
#include "color.h"

char tbl[TABLE_SIZE];

void generate_bad_character_table(char *pattern)
{
    int i, m = strlen(pattern);
    for (i = 0; i < TABLE_SIZE; ++i) {
        tbl[i] = m + 1;
    }
    for (i = 0; i < m; ++i) {
        tbl[pattern[i]] = m - i;
    }
}

char *get_bad_character_table()
{
    return tbl;
}

int search(int fd, char *buf, char *pattern, matched_line_queue *match_lines)
{
    int read_len, total = 0;
    int pattern_len = strlen(pattern);

    while ((read_len = read(fd, buf, N)) > 0) {
        match matches[MAX_MATCH_COUNT];
        int count = ssabs(buf, read_len, pattern, matches, MAX_MATCH_COUNT);

        for (int i = 0; i < count; ) {
            int end = matches[i].start;
            while (buf[end] != 0x0A && buf[end] != 0x0D) end++;

            int current_line_no = matches[i].line_no;
            int line_start = matches[i].line_start;

            int line_len = end - line_start + 1;
            int buffer_len = line_len + (strlen(MATCH_WORD_COLOR) + strlen(RESET_COLOR)) * (line_len / pattern_len);
            matched_line_queue_node *node = (matched_line_queue_node *)malloc(sizeof(matched_line_queue_node));
            node->line = (char *)malloc(sizeof(char) * buffer_len);
            sprintf(node->line, "%s%d%s:", LINE_NO_COLOR, current_line_no, RESET_COLOR);
            int print_start = line_start;
            while (i < count && current_line_no == matches[i].line_no) {
                int match_start = matches[i].start;
                int prefix_length = match_start - print_start + 1;
                char *prefix = (char *)malloc(sizeof(char) * prefix_length);

                strlcpy(prefix, buf + print_start, prefix_length);
                sprintf(node->line + strlen(node->line), "%s%s%s%s", prefix, MATCH_WORD_COLOR, pattern, RESET_COLOR);
                free(prefix);

                print_start = match_start + pattern_len;
                i++;
            }

            int suffix_length = end - print_start + 1;
            char *suffix = (char *)malloc(sizeof(char) * suffix_length);
            strlcpy(suffix, buf + print_start, suffix_length);
            sprintf(node->line + strlen(node->line), "%s", suffix);
            free(suffix);

            enqueue_matched_line(match_lines, node);
            total++;
        }

        if (read_len < N) {
            break;
        }
    }

    return total;
}

int ssabs(const char *buf, int buf_len, const char *pattern, match *matches, int max_match)
{
    int i, j = 0, m = strlen(pattern);

    int match_count = 0;
    int line_no = 1;
    int line_start = 0;
    char firstCh = pattern[0];
    char lastCh  = pattern[m - 1];
    char *tbl = get_bad_character_table();
    while (j <= buf_len - m) {
        if (lastCh == buf[j + m - 1] && firstCh == buf[j]) {
            for (i = m - 2; i >= 0 && pattern[i] == buf[j + i]; --i) {
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
        int skip = tbl[(unsigned char)buf[j + m]];
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
