#include <gsl/gsl>
#include <stdio.h>

int main() {
    auto defer = gsl::finally([] { printf("scope exited.\n"); });
    printf("main started.\n");
    return 0;
}
