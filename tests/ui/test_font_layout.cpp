#include <gtest/gtest.h>
#include "ui/TextLayout.h"

TEST(TextLayout, SingleCharVertexCount) {
    std::vector<neon::TextVertex> verts;
    neon::layoutText("1", 0, 0, 1.0f, neon::Rotation::None, 800, 600, verts);
    EXPECT_EQ(verts.size(), 60u);
}

TEST(TextLayout, SpaceProducesNoVertices) {
    std::vector<neon::TextVertex> verts;
    neon::layoutText(" ", 0, 0, 1.0f, neon::Rotation::None, 800, 600, verts);
    EXPECT_EQ(verts.size(), 0u);
}

TEST(TextLayout, TwoCharsDoubleVertices) {
    std::vector<neon::TextVertex> v1, v2;
    neon::layoutText("1", 0, 0, 1.0f, neon::Rotation::None, 800, 600, v1);
    neon::layoutText("11", 0, 0, 1.0f, neon::Rotation::None, 800, 600, v2);
    EXPECT_EQ(v2.size(), v1.size() * 2);
}

TEST(TextLayout, Rot90ChangesCoords) {
    std::vector<neon::TextVertex> vn, vr;
    neon::layoutText("0", 10, 10, 1.0f, neon::Rotation::None, 800, 600, vn);
    neon::layoutText("0", 10, 10, 1.0f, neon::Rotation::Rot90, 800, 600, vr);
    EXPECT_EQ(vn.size(), vr.size());
    bool anyDiff = false;
    for (size_t i = 0; i < vn.size(); ++i) {
        if (vn[i].x != vr[i].x || vn[i].y != vr[i].y) { anyDiff = true; break; }
    }
    EXPECT_TRUE(anyDiff);
}
