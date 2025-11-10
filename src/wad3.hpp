#pragma once

#include "plugins.h"
#include "enginedef.h"
#include <vector>
#include <string>

#include <ScopeExit/ScopeExit.h>

//Fuck microsoft
#undef min
#undef max
#include <md5.h>

class WadFile;

class WadTexture
{
public:
    WadTexture(const char * name, class WadFile* package, unsigned char* miptexData, WAD3Lump_t& lump)
    {
        m_name = name;
        m_pPackage = package;
        m_pPixels = GetPixelsFromRaw(miptexData);
        memcpy(&m_pLump, &lump, sizeof(WAD3Lump_t));
    }

    ~WadTexture()
    {
        Clear();
    }

    const char * GetName() const
    {
        return m_name.c_str();
    }
    const WadFile* Package() const
    {
        return m_pPackage;
    }
    int GetWidth() const
    {
        return m_iWidth;
    }
    int GetHeight() const
    {
        return m_iHeight;
    }
    unsigned char* GetPixels() const
    {
        return m_pPixels;
    }
    void SetPixels(unsigned char* ps, int w, int h, int bpp, int pitch)
    {
        m_pPixels = ps;
        m_iWidth = w;
        m_iHeight = h;
        m_iBpp = bpp;
        m_iPitch = pitch;
    }
    void Clear()
    {
        if (m_pPixels) {
            delete[] m_pPixels;
            m_pPixels = nullptr;
        }
        m_iWidth = 0;
        m_iHeight = 0;
        m_iBpp = 0;
        m_iPitch = 0;
    }
private:
    unsigned char* GetPixelsFromRaw(unsigned char* miptexData)
    {
        if (miptexData == nullptr)
            return nullptr;
        auto miptex = (BSPMipTexHeader_t*)miptexData;
        if (miptex->height <= 0 || miptex->height > 1024 || miptex->width <= 0 || miptex->width > 1024)
            return nullptr;
        m_iWidth = miptex->width;
        m_iHeight = miptex->height;

        const int bpp = 4;
        int size = miptex->width * miptex->height;
        int paletteOffset = miptex->offsets[0] + size + (size / 4) + (size / 16) + (size / 64) + sizeof(short);
        auto source0 = miptexData + miptex->offsets[0];
        auto palette = miptexData + paletteOffset;
        auto pixels = new unsigned char[size * bpp];
        unsigned char r, g, b, a;
        for (int i = 0; i < size; i++) {
            r = palette[source0[i] * 3];
            g = palette[source0[i] * 3 + 1];
            b = palette[source0[i] * 3 + 2];
            a = 255;
            if (miptex->name[0] == '{' && source0[i] == 255)
                r = g = b = a = 0;
            pixels[i * bpp + 0] = r;
            pixels[i * bpp + 1] = g;
            pixels[i * bpp + 2] = b;
            pixels[i * bpp + 3] = a;
        }
        m_iBpp = bpp;
        m_iPitch = bpp * m_iWidth;
        return pixels;
    }

    std::string m_name;
    WadFile* m_pPackage{};
    size_t m_iWidth{};
    size_t m_iHeight{};
    size_t m_iBpp{};
    size_t m_iPitch{};
    unsigned char* m_pPixels{};
    WAD3Lump_t m_pLump{};
};

class WadFile
{
public:

    ~WadFile()
    {
        Clear();
    }

    const unsigned char* GetMD5() const
    {
        return m_rgubMD5;
    }

    WadTexture* Get(const char *name, bool ignoreCase = true) const
    {
        if (ignoreCase)
        {
            for (auto it = m_textures.begin(); it != m_textures.end(); it++) {
                auto p = (*it);
                if (!stricmp(name, p->GetName())) {
                    return p;
                }
            }
        }
        else
        {
            for (auto it = m_textures.begin(); it != m_textures.end(); it++) {
                auto p = (*it);
                if (!strcmp(name, p->GetName())) {
                    return p;
                }
            }
        }
        return nullptr;
    }

    bool Load(const char *filePath)
    {
        Clear();

        auto hFileHandle = FILESYSTEM_ANY_OPEN(filePath, "rb");

        if (!hFileHandle)
            return false;

		SCOPE_EXIT{ FILESYSTEM_ANY_CLOSE(hFileHandle); };

        WAD3Header_t header;
        FILESYSTEM_ANY_READ(&header, sizeof(WAD3Header_t), hFileHandle);
        if (0 != memcmp(header.signature, "WAD3", 4))
            return false;

        m_lumps = new WAD3Lump_t[header.lumpsCount];
        FILESYSTEM_ANY_SEEK(hFileHandle, header.lumpsOffset, FILESYSTEM_SEEK_HEAD);
        FILESYSTEM_ANY_READ(m_lumps, header.lumpsCount * sizeof(WAD3Lump_t), hFileHandle);
        for (size_t i = 0; i < header.lumpsCount; i++) {
            unsigned char* lumpData = new unsigned char[m_lumps[i].size];
            FILESYSTEM_ANY_SEEK(hFileHandle, m_lumps[i].offset, FILESYSTEM_SEEK_HEAD);
            FILESYSTEM_ANY_READ(lumpData, m_lumps[i].size, hFileHandle);
            if (FILESYSTEM_ANY_EOF(hFileHandle))
                break;
            m_textures.push_back(new WadTexture(m_lumps[i].name, this, lumpData, m_lumps[i]));
            delete[] lumpData;
        }

        auto fileSize = FILESYSTEM_ANY_SIZE(hFileHandle);

        std::vector<unsigned char> wadBytes(fileSize);
        FILESYSTEM_ANY_SEEK(hFileHandle, 0, FILESYSTEM_SEEK_HEAD);
        FILESYSTEM_ANY_READ(wadBytes.data(), fileSize, hFileHandle);

        Chocobo1::MD5 hasher;
        hasher.addData(wadBytes.data(), wadBytes.size());

        //hash the WAD file content as MD5. 
        const auto& md5 = hasher.finalize();
        const auto& md5Array = md5.toArray();

        for (int i = 0; i < 16; ++i)
        {
            m_rgubMD5[i] = md5Array[i];
        }

        m_pHeader = header;
        return true;
    }

    void Clear()
    {
        if (m_textures.size() > 0) {
            for (auto it = m_textures.begin(); it != m_textures.end(); it++) {
                delete* it;
            }
            m_textures.clear();
        }

        if (m_lumps) {
            delete[] m_lumps;
            m_lumps = nullptr;
        }
    }
private:
    std::vector<WadTexture*> m_textures;
    WAD3Header_t m_pHeader;
    WAD3Lump_t* m_lumps{};
    unsigned char m_rgubMD5[16]{};
};