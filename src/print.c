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

    puts_with_color(filename, op.color_path);

    if (op.file_with_matches || op.group) {
        putc('\n', stdout);
    } else {
        putc(':', stdout);
    }
}

void print_line_number(match_line_node *match_line, int max_digit)
{
    char *color = NULL, sep = 0;
    switch (match_line->context) {
        case CONTEXT_NONE:
            color = op.color_line_number;
            sep = ':';
            break;
        case CONTEXT_BEFORE:
            color = op.color_before_context;
            sep = '-';
            break;
        case CONTEXT_AFTER:
            color = op.color_after_context;
            sep = '-';
            break;
    }

    if (op.group) {
        printf_with_color("%*d", color, max_digit, match_line->line_no);
    } else {
        printf_with_color("%d", color, match_line->line_no);
    }

    putc(sep, stdout);
}

void print_result(file_queue_node *current)
{
    if (op.file_with_matches || op.group) {
        print_filename(current->filename);
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
            print_filename(current->filename);
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
