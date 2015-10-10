#include <string.h>
#include <gperftools/tcmalloc.h>
#include "fjs.h"

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
            tc_free(gbetap[i]);
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
