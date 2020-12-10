#include <cstdint>
#include <cstring>
#include <iostream>

#include "MadDecoder.hxx"

using namespace std;

int main(int argc, const char** argv) {
    if (argc < 2) {
        cerr << "usage: decode_mp3 <input.mp3>" << endl;

        return 0;
    }

    MadDecoder decoder;

    if (!decoder.open(argv[1])) {
        cerr << "ERROR: unable to open " << argv[1] << endl;

        return 1;
    }

    char* buffer = new char[4 * 1024];

    uint32_t samples;
    while ((samples = decoder.decode((int16_t*)buffer, 1024)) > 0) cout.write((const char*)buffer, 4 * samples);

    delete[] buffer;
}
