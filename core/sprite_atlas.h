#pragma once
#include <cstdint>

class SpriteAtlas {
public:
    void load(const uint16_t* pixels, int sheetW, int sheetH, int spriteW, int spriteH);
    void draw(int index, int dx, int dy);

private:
    const uint16_t* pixels = nullptr;
    int sheetW = 0, sheetH = 0;
    int spriteW = 0, spriteH = 0;
    int cols = 0;
};


