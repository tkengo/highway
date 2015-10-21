#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include "file.h"
#include "util.h"
#include "log.h"

/**
 * Check if the filename is a binary file.
 */
enum file_type detect_file_type(int fd, const char *filename)
{
    if (op.stdin_redirect) {
        return FILE_TYPE_UTF8;
    }

    // Read the file content only 512-bytes from the header in order to improve speed of the
    // encoding detection. The detection accuracy of this method is not perfect but fast.
    char buf[BUF_SIZE_FOR_FILE_TYPE_CHECK];

    int read_bytes = read(fd, buf, BUF_SIZE_FOR_FILE_TYPE_CHECK);
    if (read_bytes == 0) {
        return FILE_TYPE_BINARY;
    }
    if (lseek(fd, 0, SEEK_SET) != -1) {
        log_w("%s lseek failed. You might want to search again.", filename);
    }

    int unknown = 0, utf8 = 0, euc = 0, sjis = 0;
    for (int i = 0; i < read_bytes; i++) {
        unsigned char c1 = (unsigned char)buf[i];
        if (c1 == 0x00) {
            // NULL character. This is a binary file.
            return FILE_TYPE_BINARY;
        } else {
            // ASCII detection.
            if (c1 == 0x09 || /* Horizontal tab  */
                c1 == 0x0A || /* Line feed       */
                c1 == 0x0D || /* Carriage return */
                (0x20 <= c1 && c1 < 0x7E) /* Basic Latin block */) {
                continue;
            }

            // 2-byte character detection for UTF-8.
            unsigned char c2;
            if (i + 1 < read_bytes) {
                i++;
                c2 = (unsigned char)buf[i];
                if (0xC2 <= c1 && c1 <= 0xDF &&
                    0x80 <= c2 && c2 <= 0xBF) {
                    utf8++;
                    continue;
                }
            }

            // 3-byte character detection. Only UTF-8.
            unsigned char c3;
            if (i + 1 < read_bytes) {
                i++;
                c3 = (unsigned char)buf[i];
                if ((c1 == 0xE0) &&
                    (0xA0 <= c2 && c2 <= 0xBF) &&
                    (0x80 <= c3 && c3 <= 0xBF)) {
                    utf8++;
                    continue;
                }

                if (((0xe1 <= c1 && c1 <= 0xec) || c1 == 0xee || c1 == 0xef) &&
                    ( 0x80 <= c2 && c2 <= 0xbf) &&
                    ( 0x80 <= c3 && c3 <= 0xbf)) {
                    utf8++;
                    continue;
                }

                if ((c1 == 0xed) &&
                    (0x80 <= c2 && c2 <= 0x9f) &&
                    (0x80 <= c3 && c3 <= 0xbf)) {
                    utf8++;
                    continue;
                }
            }

            // 2-byte character detection for EUC-JP or SHIFT_JIS.
            if (i + 1 < read_bytes) {
                if ((c1 == 0x8E) &&
                    (0xA1 <= c2 && c2 <= 0xDF)) {
                    euc++;
                    continue;
                }

                if ((0xA1 <= c1 && c1 <= 0xFE) &&
                    (0xA1 <= c2 && c2 <= 0xFE)) {
                    euc++;
                    continue;
                }

                if (((0x81 <= c1 && c1 <= 0x9F) || (0xE0 <= c1 && c1 <= 0xEF)) &&
                    ((0x40 <= c2 && c2 <= 0x7E) || (0x80 <= c2 && c2 <= 0xFC))) {
                    sjis++;
                    continue;
                }
            }

            // half-byte character detection. This is Shift-JIS encoding.
            if (0xA1 <= c1 && c1 <= 0xDF) {
                sjis++;
                continue;
            }

            // Unknown character.
            unknown++;
        }
    }

    if (unknown * 100 / read_bytes > 10) {
        return FILE_TYPE_BINARY;
    } else if (utf8 >= euc && utf8 >= sjis) {
        return FILE_TYPE_UTF8;
    } else if (euc >= utf8 && euc >= sjis) {
        return FILE_TYPE_EUC_JP;
    } else if (sjis >= utf8 && sjis >= euc) {
        return FILE_TYPE_SHIFT_JIS;
    } else {
        return FILE_TYPE_UTF8;
    }
}
