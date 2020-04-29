#include <cstdint>
#include <iostream>

#include "DirectoryPlayer.hxx"

using namespace std;

int main(int argc, const char** argv) {
    if (argc < 2) {
        cerr << "usage: decode_mp3 <directory>" << endl;

        return 0;
    }

    DirectoryPlayer player;

    if (!player.open(argv[1])) {
        cerr << "ERROR: unable to open " << argv[1] << endl;

        return 1;
    }

    char* buffer = new char[4 * 1024];

    uint32_t samples;
    while ((samples = player.decode((int16_t*)buffer, 1024)) > 0) cout.write((const char*)buffer, 4 * samples);

    delete[] buffer;
}
