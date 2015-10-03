#ifndef _HW_UTIL_H_
#define _HW_UTIL_H_

#define MIN(a,b) ((a) < (b)) ? (a) : (b)

char *trim(char *str);
void init_iconv();
void close_iconv();
bool stdin_redirect_from();
bool stdout_redirect_to();
void to_euc(char *in, size_t nin, char *out, size_t nout);
void to_sjis(char *in, size_t nin, char *out, size_t nout);
void to_utf8_from_euc(char *in, size_t nin, char *out, size_t nout);
void to_utf8_from_sjis(char *in, size_t nin, char *out, size_t nout);

#endif // _HW_UTIL_H_
