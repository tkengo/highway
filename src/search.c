#include <string.h>
#include "search.h"

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

int search(const char *y, int n, const char *x, match *matches)
{
    int i, j = 0, m = strlen(x);

    int count = 0;
    int line_no = 1;
    int line_start = 0;
    char firstCh = x[0];
    char lastCh  = x[m - 1];
    char *tbl = get_bad_character_table();
    while (j <= n - m) {
        if (lastCh == y[j + m - 1] && firstCh == y[j]) {
            for (i = m - 2; i >= 0 && x[i] == y[j + i]; --i) {
                if (i <= 0) {
                    // matched
                    matches[count].start = j;
                    matches[count].line_no = line_no;
                    matches[count].line_start = line_start;
                    count++;
                }
            }
        }
        int skip = tbl[(unsigned char)y[j + m]];
        for (int k = 0; k < skip; k++) {
            int t = y[j + k];
            if (t == 0x0A || t == 0x0D) {
                line_no++;
                line_start = j + k + 1;
            }
        }
        j += skip;
    }

    return count;
}
