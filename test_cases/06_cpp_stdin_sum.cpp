#include <iostream>

int main() {
    int first;
    int second;
    if (!(std::cin >> first >> second)) {
        return 1;
    }
    std::cout << first + second << '\n';
    return 0;
}
