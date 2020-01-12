#include "common.h"

int testUDP(void) {
    // TODO
}

int testTCP(void) {
    // TODO
}

int main(void)
{
    int ret;
    if ((ret = testUDP() != 0) return ret;
    if ((ret = testTCP() != 0) return ret;
    return 0;
}
