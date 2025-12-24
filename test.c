#include <stdio.h>

int main() {
    char hello[] = "hello goodbye";
    hello[6] = 'b';
    printf("%s", hello);
}