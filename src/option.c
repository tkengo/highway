#include <stdlib.h>
#include <getopt.h>
#include "help.h"
#include "highway.h"

void init_option(int argc, char **argv)
{
    bool help = false;

    static struct option longopts[] = {
        { "help", no_argument, NULL, 'h' }
    };

    int op;
    while ((op = getopt_long(argc, argv, "h", longopts, NULL)) != -1) {
        switch (op) {
            case 'h':
                help = true;
                break;
            case 'a':
            case 'b':
            case 'c':
                /* getoptの返り値は見付けたオプションである. */
                break;

            case 'd':
            case 'e':
                /* 値を取る引数の場合は外部変数optargにその値を格納する. */
                break;

                /* 以下二つのcaseは意味がないようだ.
                   getoptが直接エラーを出力してくれるから.
                   プログラムを終了するなら意味があるかも知れない */
            case ':':
                /* 値を取る引数に値がなかった場合:を返す. */
                break;

                /* getoptの引数で指定されなかったオプションを受け取ると?を返す. */
            case '?':
                usage();
                exit(1);
                break;

            default:
                usage();
                exit(1);
        }
    }

    if (help) {
        usage();
        exit(0);
    }
    /* getoptはoptindに「次に処理すべき引数のインデクスを格納している. */
    /* ここではoptindを使用してオプションの値ではない値を処理する. */
}
