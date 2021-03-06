#include <Parsers/TgxFile.h>

using namespace Sourcehold::Parsers;
using namespace Sourcehold::System;

struct TgxFile::TgxHeader {
    uint16_t width;
    uint16_t u0;
    uint16_t height;
    uint16_t u1;
};

TgxFile::TgxFile()
{
}

TgxFile::TgxFile(boost::filesystem::path path)
{
    this->LoadFromDisk(path);
}

TgxFile::~TgxFile()
{
    Unload();
}

bool TgxFile::LoadFromDisk(boost::filesystem::path path)
{
    if(!Parser::Open(path.string(), std::ifstream::in | std::ios::binary)) {
        Logger::error("PARSERS")  << "Unable to open Tgx file '" << path << "'!" << std::endl;
        return false;
    }

    TgxHeader header;
    if(!Parser::GetData(&header, sizeof(TgxHeader))) {
        Logger::error("PARSERS") << "Unable to load Tgx file header from '" << path << "'!" << std::endl;
        return false;
    }

    /* Allocate image */
    Surface surf;
    surf.AllocNew(header.width, header.height, SDL_PIXELFORMAT_RGBA4444);

    /* Calculate size */
    size_t size = Parser::GetLength() - Parser::Tell();

    char *buf = new char[size];
    Parser::GetData(buf, size);
    Parser::Close();

    /* Read image data */
    surf.LockSurface();
    ReadTgx(surf, buf, size, 0, 0, header.width, header.height, nullptr);
    surf.UnlockSurface();

    /* Convert to texture */
    Texture::AllocFromSurface(surf);

    delete [] buf;
    return true;
}

void TgxFile::Unload()
{
}

void TgxFile::ReadTgx(Surface &tex, char *buf, size_t size, uint16_t offX, uint16_t offY, uint16_t width, uint16_t height, uint16_t *pal, uint8_t color)
{
    int32_t x = 0, y = 0;
    char *end = buf + size;
    char *begin = buf;

    while(buf < end) {
        /* Read token byte */
        uint8_t b = *(uint8_t*)buf;
        buf++;
        uint8_t len = (b & 0b11111) + 1;
        uint8_t flag = b >> 5;

        switch(flag) {
        case 0b000: {
            for(uint8_t i = 0; i < len; ++i,++x) {
                uint16_t pixelColor;
                if(pal) {
                    uint8_t index = *(uint8_t*)buf;
                    buf++;
                    pixelColor = pal[256 * color + index];
                }
                else {
                    pixelColor = *reinterpret_cast<uint16_t*>(buf);
                    buf += 2;
                }
                uint8_t r,g,b,a;
                ReadPixel(pixelColor, r, g, b, a);
                tex.SetPixel(x+offX, y+offY, r, g, b, a);
            }
        }
        break;
        case 0b100: {
            y++;
            x = 0;
        }
        break;
        case 0b010: {
            uint16_t pixelColor;
            if(pal) {
                uint8_t index = *(uint8_t*)buf;
                buf++;
                pixelColor = pal[256 * color + index];
            }
            else {
                pixelColor = *reinterpret_cast<uint16_t*>(buf);
                buf += 2;
            }
            uint8_t r,g,b,a;

            ReadPixel(pixelColor, r, g, b,a );
            for(uint8_t i = 0; i < len; ++i,++x) {
                tex.SetPixel(x+offX, y+offY, r, g, b, 0xFF);
            }
        }
        break;
        case 0b001: {
            for(uint8_t i = 0; i < len; i++, x++) {
                tex.SetPixel(x+offX, y+offY, 0, 0, 0, 0);
            }
        }
        break;
        default: {
            Logger::warning("PARSERS") << "Unknown token in tgx!" << std::endl;
            return;
        }
        break;
        }
    }
}

void TgxFile::ReadPixel(uint16_t pixel, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a)
{
    a = 0xFF;
    r = ((pixel >> 10) & 0x1F) << 3;
    g = ((pixel >> 5) & 0x1F) << 3;
    b = (pixel & 0x1F) << 3;
}
