#include <string.h>
#include <ctype.h>
#include <iconv.h>

static iconv_t euc_ic;
static iconv_t sjis_ic;
static iconv_t utf8_euc_ic;
static iconv_t utf8_sjis_ic;

char *trim(char *str)
{
    while (isspace(*str)) str++;
    if (*str == '\0') {
        return str;
    }

    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    *(end + 1) = '\0';

    return str;
}

void init_iconv()
{
    euc_ic       = iconv_open("EUC-JP",    "UTF-8");
    sjis_ic      = iconv_open("SHIFT_JIS", "UTF-8");
    utf8_euc_ic  = iconv_open("UTF-8",     "EUC-JP");
    utf8_sjis_ic = iconv_open("UTF-8",     "SHIFT_JIS");
}

void close_iconv()
{
    iconv_close(euc_ic);
    iconv_close(sjis_ic);
}

void to_euc(char *in, size_t nin, char *out, size_t nout)
{
    char *ptr_in = in, *ptr_out = out;
    iconv(euc_ic, &ptr_in, &nin, &ptr_out, &nout);
}

void to_sjis(char *in, size_t nin, char *out, size_t nout)
{
    char *ptr_in = in, *ptr_out = out;
    iconv(sjis_ic, &ptr_in, &nin, &ptr_out, &nout);
}

void to_utf8_from_euc(char *in, size_t nin, char *out, size_t nout)
{
    char *ptr_in = in, *ptr_out = out;
    iconv(utf8_euc_ic, &ptr_in, &nin, &ptr_out, &nout);
}

void to_utf8_from_sjis(char *in, size_t nin, char *out, size_t nout)
{
    char *ptr_in = in, *ptr_out = out;
    iconv(utf8_sjis_ic, &ptr_in, &nin, &ptr_out, &nout);
}
