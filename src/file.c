#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include "file.h"

/**
 * Check if the filename is a binary file.
 */
bool is_binary(int fd)
{
    // Read the file content only 512-bytes from the header in order to improve speed of the
    // encoding detection. The detection accuracy of this method is not perfect but fast.
    const int BUF_SIZE = 512;
    char buf[BUF_SIZE];
    /* FILE *fp = fopen(filename, "rb"); */
    /* int actualBufSize = fread(buf, sizeof(char), actualBufSize, fp); */
    /* fclose(fp); */

    int actualBufSize = read(fd, buf, BUF_SIZE);
    if (actualBufSize == 0) {
        return true;
    }
    lseek(fd, 0, SEEK_SET);

    int unknownCharacter = 0;
    for (int i = 0; i < actualBufSize; i++) {
        unsigned char c1 = (unsigned char)buf[i];
        if (c1 == 0x00) {
            return true;
        } else {
            // ASCII detection.
            if (c1 == 0x09 || /* Horizontal tab  */
                c1 == 0x0A || /* Line feed       */
                c1 == 0x0D || /* Carriage return */
                (0x20 <= c1 && c1 < 0x7E) /* Basic Latin block */) {
                continue;
            }

            // 2-byte character detection.
            unsigned char c2;
            if (i + 1 < actualBufSize) {
                i++;
                c2 = (unsigned char)buf[i];
                if (0xC2 <= c1 && c1 <= 0xDF &&
                    0x80 <= c2 && c2 <= 0xBF) {
                    continue;
                }
            }

            // 3-byte character detection.
            unsigned char c3;
            if (i + 1 < actualBufSize) {
                i++;
                c3 = (unsigned char)buf[i];
                if ((c1 == 0xE0) &&
                    (0xA0 <= c2 && c2 <= 0xBF) &&
                    (0x80 <= c3 && c3 <= 0xBF)) {
                    continue;
                }

                if (((0xe1 <= c1 && c1 <= 0xec) || c1 == 0xee || c1 == 0xef) &&
                    ( 0x80 <= c2 && c2 <= 0xbf) &&
                    ( 0x80 <= c3 && c3 <= 0xbf)) {
                    continue;
                }

                if ((c1 == 0xed) &&
                    (0x80 <= c2 && c2 <= 0x9f) &&
                    (0x80 <= c3 && c3 <= 0xbf)) {
                    continue;
                }
            }

            // unknown character.
            unknownCharacter++;
        }
    }

    if (unknownCharacter * 100 / actualBufSize > 10) {
        return true;
    }

    return false;
}

/**
 * Check if the directory entry is ignored by the highway. The directory is ignored if it is the
 * current directory or upper directory or .git directory.
 */
bool is_ignore_directory(struct dirent *entry)
{
    bool cur = entry->d_type == DT_DIR && entry->d_namlen == 1 && entry->d_name[0] == '.';
    bool up  = entry->d_type == DT_DIR && entry->d_namlen == 2 && entry->d_name[0] == '.' && entry->d_name[1] == '.';
    bool git = entry->d_type == DT_DIR && strcmp(entry->d_name, ".git") == 0;
    bool vendor = entry->d_type == DT_DIR && strcmp(entry->d_name, "vendor") == 0;

    return cur || up || git || vendor;
}

bool is_directory(struct dirent *entry)
{
    return entry->d_type == DT_DIR;
}

bool is_search_target(struct dirent *entry)
{
    return entry->d_type == DT_REG || entry->d_type == DT_LNK;
}

/* void find_target_files(file_queue *queue, char *dirname) */
/* { */
/*     DIR *dir = opendir(dirname); */
/*     struct dirent *entry; */
/*     while ((entry = readdir(dir)) != NULL) { */
/*         if (is_ignore_directory(entry)) { */
/*             continue; */
/*         } */
/*  */
/*         char buf[256]; */
/*         sprintf(buf, "%s/%s", dirname, entry->d_name); */
/*  */
/*         if (is_directory(entry)) { */
/*             find_target_files(queue, buf); */
/*         } else if (is_search_target(entry)) { */
/*             enqueue_file(queue, buf); */
/*         } */
/*     } */
/*     closedir(dir); */
/* } */
