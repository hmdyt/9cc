#include "9cc.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        perror("引数の個数が正しくありません");
        return 1;
    }

    // トークナイズしてパースする
    Node *node = parse(argv[1]);

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    // 抽象構文木を下りながらコード生成
    gen(node);

    // スタックトップに式全体の値が残っているはずなので
    // それをRAXにロードして関数からの返り値とする
    printf("  pop rax\n");
    printf("  ret\n");
    return 0;
}