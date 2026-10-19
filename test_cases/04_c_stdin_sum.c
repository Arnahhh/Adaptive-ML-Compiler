#include <stdio.h>

int main(void) {
    int first;
    int second;
    if (scanf("%d %d", &first, &second) != 2) {
        return 1;
    }
    printf("%d\n", first + second);
    return 0;
}
