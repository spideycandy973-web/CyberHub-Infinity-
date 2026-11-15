#pragma once
#include "ProceduralFont.h"
#include <vector>

namespace neon {

enum class Rotation { None, Rot90, Rot180, Rot270 };

struct TextVertex {
    float x, y;
};

inline void layoutText(const char* text, float ox, float oy, float scale,
                       Rotation rot, float screenW, float screenH,
                       std::vector<TextVertex>& out) {
    float cx = ox;
    float cy = oy;
    const float charW = 6.0f * scale;
    const float charH = 8.0f * scale;

    auto transform = [&](float px, float py, float& tx, float& ty) {
        switch (rot) {
            case Rotation::None:   tx = px;           ty = py;            break;
            case Rotation::Rot90:  tx = screenW - py;  ty = px;            break;
            case Rotation::Rot180: tx = screenW - px;  ty = screenH - py;  break;
            case Rotation::Rot270: tx = py;            ty = screenH - px;  break;
        }
    };

    for (int i = 0; text[i]; ++i) {
        char c = text[i];
        if (c == '\n') { cx = ox; cy += charH; continue; }
        Glyph g = getGlyph(c);
        for (int col = 0; col < 5; ++col) {
            for (int row = 0; row < 7; ++row) {
                if (!(g.cols[col] & (1u << row))) continue;
                float x0 = cx + col * scale;
                float y0 = cy + row * scale;
                float x1 = x0 + scale;
                float y1 = y0 + scale;
                float a, b, c2, d, e, f2, g2, h;
                transform(x0, y0, a, b);
                transform(x1, y0, c2, d);
                transform(x0, y1, e, f2);
                transform(x1, y1, g2, h);
                out.push_back({a,  b});
                out.push_back({c2, d});
                out.push_back({e,  f2});
                out.push_back({c2, d});
                out.push_back({g2, h});
                out.push_back({e,  f2});
            }
        }
        cx += charW;
    }
}

} // namespace neon
