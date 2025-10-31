//
// Created by Gambler developers on 31.10.2025.
//

#ifndef GAMBLER_SEQUENCEROPTIONS_H
#define GAMBLER_SEQUENCEROPTIONS_H

#include <array>

struct SequencerOptions {
    std::array<int, 12> scale = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B

    void setScale(int c, int db, int d, int eb, int e, int f, int gb, int g, int ab, int a, int bb, int b) {
        scale[0] = c;
        scale[1] = db;
        scale[2] = d;
        scale[3] = eb;
        scale[4] = e;
        scale[5] = f;
        scale[6] = gb;
        scale[7] = g;
        scale[8] = ab;
        scale[9] = a;
        scale[10] = bb;
        scale[11] = b;
    }
};

#endif //GAMBLER_SEQUENCEROPTIONS_H
