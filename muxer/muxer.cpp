#include <iostream>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

int main()
{
    std::cout << "Hello World!\n";
    printf("%s\n", avcodec_configuration())
}