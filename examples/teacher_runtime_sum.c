#include <stdio.h>

int main(void) {
    int count;
    int value;
    long long total = 0;

    if (scanf("%d", &count) != 1 || count < 0) {
        return 1;
    }

    for (int i = 0; i < count; i++) {
        if (scanf("%d", &value) != 1) {
            return 1;
        }
        total += value;
    }

    printf("Sum = %lld\n", total);
    return 0;
}
