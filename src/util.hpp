#ifndef __GAME_UTIL
#define __GAME_UTIL
#include "engine/pair.hpp"

template <typename T>
int sign(T val) {
    return (T(0) < val) - (val < T(0));
};

int checkLineIntersection(Pair const& p0,
                          Pair const& p1,
                          Pair const& p2,
                          Pair const& p3,
                          Pair& out,
                          double epsilon = 0);

bool checkLineSweep(
        Pair const& a1,
        Pair const& a2,
        Pair const& b1,
        Pair const& b2,
        Pair const& c,
        Pair & out1,
        Pair & out2,
        double epsilon = 0);

#define M_PI 3.14159265358979323846 /* pi */

#endif
