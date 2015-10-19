#include <stdio.h>
#include <string.h>
#include <math.h>
#include "print.h"
#include "option.h"
#include "util.h"
#include "color.h"

void print_filename(const char *filename)
{
    if (op.stdin_redirect) {
        return;
    }

    if (op.color) {
        printf("%s%s%s", FILENAME_COLOR, filename, RESET_COLOR);
    } else {
        fputs(filename, stdout);
    }

    if (op.file_with_matches || op.group) {
        putc('\n', stdout);
    } else {
        putc(':', stdout);
    }
}

void print_line_number(match_line_node *match_line, int max_digit)
{
    if (op.color) {
        // Print colorized line number.
        switch (match_line->context) {
            case CONTEXT_NONE:
                if (op.group) {
                    printf("%s%*d%s", LINE_NO_COLOR, max_digit, match_line->line_no, RESET_COLOR);
                } else {
                    printf("%s%d%s", LINE_NO_COLOR, match_line->line_no, RESET_COLOR);
                }
                break;
            case CONTEXT_BEFORE:
                if (op.group) {
                    printf("%s%*d%s", CONTEXT_BEFORE_LINE_NO_COLOR, max_digit, match_line->line_no, RESET_COLOR);
                } else {
                    printf("%s%d%s", CONTEXT_BEFORE_LINE_NO_COLOR, match_line->line_no, RESET_COLOR);
                }
                break;
            case CONTEXT_AFTER:
                if (op.group) {
                    printf("%s%*d%s", CONTEXT_AFTER_LINE_NO_COLOR, max_digit, match_line->line_no, RESET_COLOR);
                } else {
                    printf("%s%d%s", CONTEXT_AFTER_LINE_NO_COLOR, match_line->line_no, RESET_COLOR);
                }
                break;
        }
    } else {
        // Print no-colorized line number.
        if (op.group) {
            printf("%*d", max_digit, match_line->line_no);
        } else {
            printf("%d", match_line->line_no);
        }
    }

    switch (match_line->context) {
        case CONTEXT_NONE:
            putc(':', stdout);
            break;
        case CONTEXT_BEFORE:
            putc('-', stdout);
            break;
        case CONTEXT_AFTER:
            putc('-', stdout);
            break;
    }
}

void print_result(const char *filename, file_queue_node *current)
{
    if (op.file_with_matches || op.group) {
        print_filename(filename);
    }

    // If `file_with_matches` option is available, match results don't print on console.
    if (op.file_with_matches) {
        return;
    }

    match_line_node *match_line;
    int max_digit = log10(current->match_lines->max_line_no) + 1;

    // Show all matched line on console in the current file.
    while ((match_line = dequeue_match_line(current->match_lines)) != NULL) {
        if (!op.group) {
            print_filename(filename);
        }

        if (op.show_line_number) {
            print_line_number(match_line, max_digit);
        }

        if (current->t == FILE_TYPE_UTF8) {
            fputs(match_line->line, stdout);
        } else {
            const int line_len = strlen(match_line->line), utf8_len_guess = line_len * 2;
            char out[utf8_len_guess];
            memset(out, 0, utf8_len_guess);
            if (current->t == FILE_TYPE_EUC_JP) {
                to_utf8_from_euc(match_line->line, line_len, out, utf8_len_guess);
            } else if (current->t == FILE_TYPE_SHIFT_JIS) {
                to_utf8_from_sjis(match_line->line, line_len, out, utf8_len_guess);
            }
            fputs(out, stdout);
        }
        putc('\n', stdout);
    }
}
