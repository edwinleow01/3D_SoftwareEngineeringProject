#include "pch.h"
#include <glew.h>
#include <fstream>
#include <vector>
#include <cstdint>
#include <iostream>
#include <cstring>

// -------------------------------------------------------------
// Minimal DDS structures
// -------------------------------------------------------------
struct DDS_PIXELFORMAT
{
    uint32_t size;
    uint32_t flags;
    uint32_t fourCC;
    uint32_t rgbBitCount;
    uint32_t rMask;
    uint32_t gMask;
    uint32_t bMask;
    uint32_t aMask;
};

struct DDS_HEADER
{
    uint32_t         magic;
    uint32_t         size;
    uint32_t         flags;
    uint32_t         height;
    uint32_t         width;
    uint32_t         pitchOrLinearSize;
    uint32_t         depth;
    uint32_t         mipMapCount;
    uint32_t         reserved1[11];
    DDS_PIXELFORMAT  ddspf;
    uint32_t         caps;
    uint32_t         caps2;
    uint32_t         caps3;
    uint32_t         caps4;
    uint32_t         reserved2;
};

// DX10 extension header (BC6 / BC7 and modern formats)
struct DDS_HEADER_DXT10
{
    uint32_t dxgiFormat;
    uint32_t resourceDimension;
    uint32_t miscFlag;
    uint32_t arraySize;
    uint32_t miscFlags2;
};

// -------------------------------------------------------------
// DXGI format IDs we care about
// -------------------------------------------------------------
enum DXGI_FORMAT : uint32_t
{
    DXGI_FORMAT_BC1_UNORM = 71,
    DXGI_FORMAT_BC1_UNORM_SRGB = 72,
    DXGI_FORMAT_BC2_UNORM = 74,
    DXGI_FORMAT_BC2_UNORM_SRGB = 75,
    DXGI_FORMAT_BC3_UNORM = 77,
    DXGI_FORMAT_BC3_UNORM_SRGB = 78,
    DXGI_FORMAT_BC4_UNORM = 80,
    DXGI_FORMAT_BC5_UNORM = 83,
    DXGI_FORMAT_BC7_UNORM = 98,
    DXGI_FORMAT_BC7_UNORM_SRGB = 99
};

// Legacy FourCC codes (older DDS files — what most exporters produce by default)
static constexpr uint32_t FOURCC_DXT1 = 0x31545844; // "DXT1"
static constexpr uint32_t FOURCC_DXT3 = 0x33545844; // "DXT3"
static constexpr uint32_t FOURCC_DXT5 = 0x35545844; // "DXT5"
static constexpr uint32_t FOURCC_ATI1 = 0x31495441; // "ATI1" (BC4)
static constexpr uint32_t FOURCC_ATI2 = 0x32495441; // "ATI2" (BC5)
static constexpr uint32_t FOURCC_DX10 = 0x30315844; // "DX10" extended header

// -------------------------------------------------------------
// Map DXGI format -> GL internal format  (DX10 path)
// -------------------------------------------------------------
static GLenum DXGIToGL(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_BC1_UNORM:      return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    case DXGI_FORMAT_BC1_UNORM_SRGB: return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
    case DXGI_FORMAT_BC2_UNORM:      return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    case DXGI_FORMAT_BC2_UNORM_SRGB: return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
    case DXGI_FORMAT_BC3_UNORM:      return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    case DXGI_FORMAT_BC3_UNORM_SRGB: return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
    case DXGI_FORMAT_BC4_UNORM:      return GL_COMPRESSED_RED_RGTC1;
    case DXGI_FORMAT_BC5_UNORM:      return GL_COMPRESSED_RG_RGTC2;
    case DXGI_FORMAT_BC7_UNORM:      return GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
    case DXGI_FORMAT_BC7_UNORM_SRGB: return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB;
    default:                         return 0;
    }
}

// -------------------------------------------------------------
// Map legacy FourCC -> GL internal format  (non-DX10 path)
// Returns 0 if unsupported.
// -------------------------------------------------------------
static GLenum LegacyFourCCToGL(uint32_t fourCC)
{
    switch (fourCC)
    {
    case FOURCC_DXT1: return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    case FOURCC_DXT3: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    case FOURCC_DXT5: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    case FOURCC_ATI1: return GL_COMPRESSED_RED_RGTC1;   // BC4
    case FOURCC_ATI2: return GL_COMPRESSED_RG_RGTC2;    // BC5
    default:          return 0;
    }
}

// -------------------------------------------------------------
// Helper: block size in bytes for a given GL compressed format
// -------------------------------------------------------------
static uint32_t BlockSize(GLenum fmt)
{
    switch (fmt)
    {
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RED_RGTC1:
        return 8;   // BC1 / BC4 — 8 bytes per 4x4 block
    default:
        return 16;  // BC2 / BC3 / BC5 / BC7 — 16 bytes per 4x4 block
    }
}

// -------------------------------------------------------------
// Main loader
// -------------------------------------------------------------
GLuint LoadDDS(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
    {
        std::cerr << "[DDS] Failed to open: " << path << "\n";
        return 0;
    }

    // ------------------------------------------------------------------
    // Read base header
    // ------------------------------------------------------------------
    DDS_HEADER header{};
    f.read(reinterpret_cast<char*>(&header), sizeof(DDS_HEADER));

    if (header.magic != 0x20534444) // " DDS"
    {
        std::cerr << "[DDS] Invalid magic in: " << path << "\n";
        return 0;
    }

    // ------------------------------------------------------------------
    // Resolve GL internal format
    // Two paths:
    //   (A) DX10 extended header  — modern tools (Texconv default)
    //   (B) Legacy FourCC         — older tools / Figma exports
    // ------------------------------------------------------------------
    GLenum internalFmt = 0;
    size_t dataOffset = sizeof(DDS_HEADER);

    if (header.ddspf.fourCC == FOURCC_DX10)
    {
        // --- Path A: DX10 ---
        DDS_HEADER_DXT10 dx10{};
        f.read(reinterpret_cast<char*>(&dx10), sizeof(dx10));
        dataOffset += sizeof(DDS_HEADER_DXT10);

        internalFmt = DXGIToGL(static_cast<DXGI_FORMAT>(dx10.dxgiFormat));
        if (!internalFmt)
        {
            std::cerr << "[DDS] Unsupported DX10 DXGI format " << dx10.dxgiFormat << " in: " << path << "\n";
            return 0;
        }
    }
    else
    {
        // --- Path B: Legacy FourCC (DXT1 / DXT3 / DXT5 / ATI1 / ATI2) ---
        internalFmt = LegacyFourCCToGL(header.ddspf.fourCC);
        if (!internalFmt)
        {
            std::cerr << "[DDS] Unsupported legacy FourCC 0x" << std::hex << header.ddspf.fourCC << std::dec << " in: " << path << "\n";
            return 0;
        }
    }

    // ------------------------------------------------------------------
    // Read pixel data
    // ------------------------------------------------------------------
    f.seekg(0, std::ios::end);
    size_t fileSize = static_cast<size_t>(f.tellg());
    size_t dataSize = fileSize - dataOffset;

    f.seekg(static_cast<std::streamoff>(dataOffset), std::ios::beg);
    std::vector<uint8_t> buffer(dataSize);
    f.read(reinterpret_cast<char*>(buffer.data()), dataSize);
    f.close();

    // ------------------------------------------------------------------
    // Upload to GPU
    // ------------------------------------------------------------------
    const uint32_t width = header.width;
    const uint32_t height = header.height;
    const uint32_t mips = (header.mipMapCount > 0) ? header.mipMapCount : 1;
    const uint32_t blkSz = BlockSize(internalFmt);

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    size_t   offset = 0;
    uint32_t w = width, h = height;

    for (uint32_t i = 0; i < mips; ++i)
    {
        uint32_t mipSize = ((w + 3) / 4) * ((h + 3) / 4) * blkSz;

        // Guard against reading past the buffer (malformed files)
        if (offset + mipSize > dataSize)
        {
            std::cerr << "[DDS] Truncated mip data at level " << i << " in: " << path << "\n";
            break;
        }

        glCompressedTexImage2D(GL_TEXTURE_2D, i, internalFmt, w, h, 0, mipSize, buffer.data() + offset);

        GLenum err = glGetError();
        if (err != GL_NO_ERROR)
        {
            std::cerr << "[DDS] glCompressedTexImage2D error " << err << " at mip " << i << " in: " << path << "\n";
            glDeleteTextures(1, &tex);
            return 0;
        }

        offset += mipSize;
        w = std::max(1u, w / 2);
        h = std::max(1u, h / 2);
    }

    // ------------------------------------------------------------------
    // Sampler parameters
    // ------------------------------------------------------------------
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mips > 1) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    std::cout << "[DDS] Loaded " << path << " (" << width << "x" << height << ", " << mips << " mip(s))\n";
    return tex;
}