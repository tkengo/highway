#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "common.h"
#include "hwmalloc.h"
#include "regex.h"
#include "log.h"

static pthread_mutex_t onig_mutex;
static regex_t **reg;
static enum file_type current;

/**
 * Initialize onigmo and mutex for onigmo.
 */
bool onig_init_wrap()
{
    if (pthread_mutex_init(&onig_mutex, NULL)) {
        log_e("A condition variable for file finding was not able to initialize.");
        return false;
    }

    if (onig_init() != 0) {
        log_e("Onigmo initialization was failed.");
        return false;
    }

    reg = (regex_t **)tc_calloc(op.worker, sizeof(regex_t *));

    return true;
}

/**
 * Release pointers for onigmo, and do end process of onigmo.
 */
void onig_end_wrap()
{
    for (int i = 0; i < op.worker; i++) {
        if (reg[i] != NULL) {
            onig_free(reg[i]);
        }
    }
    tc_free(reg);

    pthread_mutex_destroy(&onig_mutex);
    onig_end();
}

/**
 * Thread safety onig_new.
 */
regex_t *onig_new_wrap(const char *pattern, enum file_type t, bool ignore_case, int thread_no)
{
    if (current == t && reg[thread_no] != NULL) {
        return reg[thread_no];
    }

    OnigErrorInfo einfo;
    OnigEncodingType *enc;
    UChar *p = (UChar *)pattern;

    switch (t) {
        case FILE_TYPE_EUC_JP:
            enc = ONIG_ENCODING_EUC_JP;
            break;
        case FILE_TYPE_SHIFT_JIS:
            enc = ONIG_ENCODING_SJIS;
            break;
        default:
            enc = ONIG_ENCODING_UTF8;
            break;
    }
    pthread_mutex_lock(&onig_mutex);
    long option = ignore_case ? ONIG_OPTION_IGNORECASE : ONIG_OPTION_DEFAULT;
    int r = onig_new(&reg[thread_no], p, p + strlen(pattern), option, enc, ONIG_SYNTAX_DEFAULT, &einfo);
    current = t;
    pthread_mutex_unlock(&onig_mutex);

    if (r != ONIG_NORMAL) {
        return NULL;
    }

    return reg[thread_no];
}

bool regex(const char *buf, size_t search_len, const char *pattern, enum file_type t, match *m, int thread_no)
{
    OnigRegion *region = onig_region_new();

    // Get the compiled regular expression. Actually, onig_new method is not safety multiple-thread,
    // but this wrapper method is implemented in thread safety.
    regex_t *reg = onig_new_wrap(pattern, t, op.ignore_case, thread_no);
    if (reg == NULL) {
        return false;
    }

    bool res = false;
    size_t offset = 0;

    while (1) {
        const unsigned char *p = (unsigned char *)buf + offset,
                            *start = p,
                            *end   = start + search_len - offset,
                            *range = end;
        if (onig_search(reg, p, end, start, range, region, ONIG_OPTION_NONE) >= 0) {
            m->start = region->beg[0] + offset;
            m->end   = region->end[0] + offset;

            if (op.word_regex && !is_word_match(buf, search_len, m)) {
                offset = m->end + 1;
            } else {
                res = true;
                break;
            }
        } else {
            break;
        }
    }

    onig_region_free(region, 1);
    return res;
}
