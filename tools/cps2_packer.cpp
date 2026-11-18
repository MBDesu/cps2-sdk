#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "cps2_crypt.h"
#include "default_z80_driver.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::vector<uint8_t> readFile(const fs::path &path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open input file: " + path.string());
  }
  const auto size = file.tellg();
  file.seekg(0, std::ios::beg);
  std::vector<uint8_t> buf(static_cast<size_t>(size));
  if (size > 0) {
    file.read(reinterpret_cast<char *>(buf.data()), size);
  }
  return buf;
}

void writeFile(const fs::path &path, const uint8_t *data, size_t size) {
  std::ofstream file(path, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open output file: " + path.string());
  }
  file.write(reinterpret_cast<const char *>(data),
             static_cast<std::streamsize>(size));
}

uint16_t readBE16(const uint8_t *p) {
  return static_cast<uint16_t>((p[0] << 8) | p[1]);
}

uint32_t readBE32(const uint8_t *p) {
  return (static_cast<uint32_t>(p[0]) << 24) |
         (static_cast<uint32_t>(p[1]) << 16) |
         (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}

std::array<uint8_t, 20> encodeKeyFile(const std::array<uint16_t, 10> &words) {
  std::array<uint8_t, 20> out{};
  for (int b = 0; b < 10 * 16; ++b) {
    const int bit = (317 - b) % 160;
    if ((words[b / 16] & (0x8000u >> (b % 16))) != 0) {
      out[bit / 8] |= static_cast<uint8_t>(1u << ((bit ^ 7) % 8));
    }
  }
  return out;
}

void appendTile8x8(std::vector<uint8_t> &out,
                   const std::array<uint8_t, 64> &px) {
  const size_t base = out.size();
  out.resize(base + 128, 0x0f);
  for (int y = 0; y < 8; ++y) {
    for (int x = 0; x < 8; ++x) {
      const uint8_t c = px[y * 8 + x] & 0x0f;
      out[base + y * 16 + x] = c;
      out[base + y * 16 + 8 + x] = c;
    }
  }
}

void appendTile16x16(std::vector<uint8_t> &out,
                     const std::array<uint8_t, 256> &px) {
  const size_t base = out.size();
  out.resize(base + 256, 0x0f);
  for (int i = 0; i < 256; ++i) {
    out[base + i] = px[i] & 0x0f;
  }
}

void appendTile32x32(std::vector<uint8_t> &out,
                     const std::array<uint8_t, 1024> &px) {
  const size_t base = out.size();
  out.resize(base + 1024, 0x0f);
  for (int i = 0; i < 1024; ++i) {
    out[base + i] = px[i] & 0x0f;
  }
}

std::vector<uint8_t> buildBuiltinFontRegion() {
  std::vector<uint8_t> region;
  region.reserve(128 * 128);

  std::array<uint8_t, 64> blank{};
  blank.fill(0x0f);

  std::array<uint8_t, 64> checker{};
  for (int y = 0; y < 8; ++y) {
    for (int x = 0; x < 8; ++x) {
      checker[y * 8 + x] = ((x ^ y) & 1) ? 0x01 : 0x0f;
    }
  }

  struct Glyph {
    char ch;
    std::array<uint8_t, 8> rows;
  };

  static const Glyph kGlyphs[] = {
      {'!', {0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00}},
      {'+', {0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x00, 0x00}},
      {'-', {0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00}},
      {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00}},
      {'/', {0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0x80, 0x00}},
      {':', {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00}},
      {'0', {0x3c, 0x66, 0x6e, 0x76, 0x66, 0x66, 0x3c, 0x00}},
      {'1', {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x00}},
      {'2', {0x3c, 0x66, 0x06, 0x1c, 0x30, 0x66, 0x7e, 0x00}},
      {'3', {0x3c, 0x66, 0x06, 0x1c, 0x06, 0x66, 0x3c, 0x00}},
      {'4', {0x0e, 0x1e, 0x36, 0x66, 0x7f, 0x06, 0x0f, 0x00}},
      {'5', {0x7e, 0x60, 0x7c, 0x06, 0x06, 0x66, 0x3c, 0x00}},
      {'6', {0x1c, 0x30, 0x60, 0x7c, 0x66, 0x66, 0x3c, 0x00}},
      {'7', {0x7e, 0x66, 0x0c, 0x18, 0x18, 0x18, 0x18, 0x00}},
      {'8', {0x3c, 0x66, 0x66, 0x3c, 0x66, 0x66, 0x3c, 0x00}},
      {'9', {0x3c, 0x66, 0x66, 0x3e, 0x06, 0x0c, 0x38, 0x00}},
      {'A', {0x18, 0x3c, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x00}},
      {'B', {0x7c, 0x66, 0x66, 0x7c, 0x66, 0x66, 0x7c, 0x00}},
      {'C', {0x3c, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3c, 0x00}},
      {'D', {0x78, 0x6c, 0x66, 0x66, 0x66, 0x6c, 0x78, 0x00}},
      {'E', {0x7e, 0x62, 0x68, 0x78, 0x68, 0x62, 0x7e, 0x00}},
      {'F', {0x7e, 0x62, 0x68, 0x78, 0x68, 0x60, 0xf0, 0x00}},
      {'G', {0x3c, 0x66, 0x60, 0x60, 0x6e, 0x66, 0x3e, 0x00}},
      {'H', {0x66, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x66, 0x00}},
      {'I', {0x3c, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00}},
      {'J', {0x1e, 0x0c, 0x0c, 0x0c, 0x6c, 0x6c, 0x38, 0x00}},
      {'K', {0x66, 0x6c, 0x78, 0x70, 0x78, 0x6c, 0x66, 0x00}},
      {'L', {0x60, 0x60, 0x60, 0x60, 0x60, 0x62, 0x7e, 0x00}},
      {'M', {0x63, 0x77, 0x7f, 0x6b, 0x63, 0x63, 0x63, 0x00}},
      {'N', {0x66, 0x76, 0x7e, 0x7e, 0x6e, 0x66, 0x66, 0x00}},
      {'O', {0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00}},
      {'P', {0x7c, 0x66, 0x66, 0x7c, 0x60, 0x60, 0xf0, 0x00}},
      {'Q', {0x3c, 0x66, 0x66, 0x66, 0x6e, 0x3c, 0x0e, 0x00}},
      {'R', {0x7c, 0x66, 0x66, 0x7c, 0x6c, 0x66, 0x66, 0x00}},
      {'S', {0x3c, 0x66, 0x60, 0x3c, 0x06, 0x66, 0x3c, 0x00}},
      {'T', {0x7e, 0x5a, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00}},
      {'U', {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00}},
      {'V', {0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00}},
      {'W', {0x63, 0x63, 0x63, 0x6b, 0x7f, 0x77, 0x63, 0x00}},
      {'X', {0x66, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x66, 0x00}},
      {'Y', {0x66, 0x66, 0x66, 0x3c, 0x18, 0x18, 0x3c, 0x00}},
      {'Z', {0x7e, 0x66, 0x0c, 0x18, 0x30, 0x66, 0x7e, 0x00}},
  };

  for (int code = 0; code < 128; ++code) {
    if (code == 1) {
      appendTile8x8(region, checker);
      continue;
    }
    const Glyph *found = nullptr;
    for (const auto &g : kGlyphs) {
      if (static_cast<int>(g.ch) == code) {
        found = &g;
        break;
      }
    }
    if (!found) {
      appendTile8x8(region, blank);
      continue;
    }

    std::array<uint8_t, 64> px{};
    for (int y = 0; y < 8; ++y) {
      const uint8_t bits = found->rows[y];
      for (int x = 0; x < 8; ++x) {
        px[y * 8 + x] = ((bits >> (7 - x)) & 1) ? 0x01 : 0x0f;
      }
    }
    appendTile8x8(region, px);
  }

  return region;
}

std::vector<uint8_t> buildElfCodeMask(const fs::path &elfPath,
                                      size_t romByteSize) {
  const auto elf = readFile(elfPath);
  if (elf.size() < 52 || elf[0] != 0x7f || elf[1] != 'E' || elf[2] != 'L' ||
      elf[3] != 'F') {
    throw std::runtime_error("Invalid ELF file: " + elfPath.string());
  }

  const uint32_t e_shoff = readBE32(&elf[32]);
  const uint16_t e_shentsize = readBE16(&elf[46]);
  const uint16_t e_shnum = readBE16(&elf[48]);

  const size_t wordCount = (romByteSize + 1) / 2;
  std::vector<uint8_t> isCode(wordCount, 0);

  // Reset SSP & PC ($000000..$000007) are fetched via fetchInstruction;
  // Exception & IRQ vectors ($000008..$0003FF) are fetched via data readLong.
  for (size_t w = 0; w < std::min<size_t>(4, wordCount); ++w) {
    isCode[w] = 1;
  }

  if (e_shoff == 0 || e_shentsize < 40 ||
      static_cast<size_t>(e_shoff) +
              static_cast<size_t>(e_shnum) * e_shentsize >
          elf.size()) {
    return isCode;
  }

  struct Section {
    uint32_t type, flags, addr, offset, size, link;
  };
  std::vector<Section> sections;
  sections.reserve(e_shnum);
  for (uint16_t i = 0; i < e_shnum; ++i) {
    const size_t off = e_shoff + static_cast<size_t>(i) * e_shentsize;
    sections.push_back({
        readBE32(&elf[off + 4]),
        readBE32(&elf[off + 8]),
        readBE32(&elf[off + 12]),
        readBE32(&elf[off + 16]),
        readBE32(&elf[off + 20]),
        readBE32(&elf[off + 24]),
    });
  }

  for (const auto &sec : sections) {
    if ((sec.flags & 0x4u) != 0 && sec.type == 1) {
      const uint32_t start = std::max<uint32_t>(sec.addr, 0x400);
      const uint32_t end = std::min<uint32_t>(
          sec.addr + sec.size, static_cast<uint32_t>(romByteSize));
      for (uint32_t a = start; a + 1 < end && (a / 2) < wordCount; a += 2) {
        isCode[a / 2] = 1;
      }
    }
  }

  return isCode;
}

struct PngSheetSpec {
  std::string symbol;
  fs::path pngPath;
  int tileSize = 16;
  bool isSprite = false;
};

struct PackedSheet {
  std::string symbol;
  uint32_t firstTile = 0;
  uint32_t tileCount = 0;
  std::array<uint16_t, 16> palette{};
};

PackedSheet importPngSheet(const PngSheetSpec &spec,
                           std::vector<uint8_t> &decodedGfx,
                           size_t baseOffset) {
  int w = 0, h = 0, channels = 0;
  unsigned char *rgba =
      stbi_load(spec.pngPath.string().c_str(), &w, &h, &channels, 4);
  if (!rgba) {
    throw std::runtime_error("Failed to load PNG: " + spec.pngPath.string());
  }

  if (w % spec.tileSize != 0 || h % spec.tileSize != 0) {
    stbi_image_free(rgba);
    throw std::runtime_error("PNG dimensions must be multiples of tile size: " +
                             spec.pngPath.string());
  }

  PackedSheet result;
  result.symbol = spec.symbol;
  const uint32_t stride = (spec.tileSize == 8)    ? 128u
                          : (spec.tileSize == 16) ? 256u
                                                  : 1024u;
  const size_t relOffset = decodedGfx.size() - baseOffset;
  const size_t rem = relOffset % stride;
  if (rem != 0) {
    decodedGfx.resize(decodedGfx.size() + (stride - rem), 0x0f);
  }
  result.firstTile =
      static_cast<uint32_t>((decodedGfx.size() - baseOffset) / stride);

  std::vector<uint16_t> pal;
  pal.push_back(0x0000);
  result.palette.fill(0x0000);

  auto quantizeColor = [&](uint8_t r, uint8_t g, uint8_t b,
                           uint8_t a) -> uint8_t {
    if (a < 128) {
      return 0x0f;
    }
    const uint16_t c12 = static_cast<uint16_t>(0xf000u | ((r >> 4) << 8) |
                                               ((g >> 4) << 4) | (b >> 4));
    for (size_t i = 0; i < pal.size(); ++i) {
      if (pal[i] == c12) {
        return static_cast<uint8_t>(i);
      }
    }
    if (pal.size() >= 15) {
      return 14;
    }
    const uint8_t idx = static_cast<uint8_t>(pal.size());
    pal.push_back(c12);
    result.palette[idx] = c12;
    return idx;
  };

  const int tilesX = w / spec.tileSize;
  const int tilesY = h / spec.tileSize;
  for (int ty = 0; ty < tilesY; ++ty) {
    for (int tx = 0; tx < tilesX; ++tx) {
      if (spec.tileSize == 8) {
        std::array<uint8_t, 64> tile{};
        for (int y = 0; y < 8; ++y) {
          for (int x = 0; x < 8; ++x) {
            const size_t p = ((ty * 8 + y) * w + (tx * 8 + x)) * 4;
            tile[y * 8 + x] = quantizeColor(rgba[p + 0], rgba[p + 1],
                                            rgba[p + 2], rgba[p + 3]);
          }
        }
        appendTile8x8(decodedGfx, tile);
      } else if (spec.tileSize == 16) {
        std::array<uint8_t, 256> tile{};
        for (int y = 0; y < 16; ++y) {
          for (int x = 0; x < 16; ++x) {
            const size_t p = ((ty * 16 + y) * w + (tx * 16 + x)) * 4;
            tile[y * 16 + x] = quantizeColor(rgba[p + 0], rgba[p + 1],
                                             rgba[p + 2], rgba[p + 3]);
          }
        }
        appendTile16x16(decodedGfx, tile);
      } else {
        std::array<uint8_t, 1024> tile{};
        for (int y = 0; y < 32; ++y) {
          for (int x = 0; x < 32; ++x) {
            const size_t p = ((ty * 32 + y) * w + (tx * 32 + x)) * 4;
            tile[y * 32 + x] = quantizeColor(rgba[p + 0], rgba[p + 1],
                                             rgba[p + 2], rgba[p + 3]);
          }
        }
        appendTile32x32(decodedGfx, tile);
      }
      result.tileCount++;
    }
  }

  stbi_image_free(rgba);
  return result;
}

void shuffle64(uint64_t *buf, size_t len) {
  if (len <= 2) {
    return;
  }
  len /= 2;
  for (size_t i = 0; i < len / 2; ++i) {
    std::swap(buf[len / 2 + i], buf[len + i]);
  }
  shuffle64(buf, len);
  shuffle64(buf + len, len);
}

void writeInterleavedGfxRoms(const std::vector<uint8_t> &decodedGfx,
                             const fs::path &outDir,
                             const std::string &romName) {
  constexpr size_t kTotalGfxBytes = 0x2000000;
  std::vector<uint8_t> padded(kTotalGfxBytes * 2, 0x0f);
  const size_t copyPixels = std::min(decodedGfx.size(), padded.size());
  if (copyPixels > 0) {
    std::memcpy(padded.data(), decodedGfx.data(), copyPixels);
  }

  std::vector<uint64_t> planarWords(kTotalGfxBytes / sizeof(uint64_t), 0);
  uint8_t *planar = reinterpret_cast<uint8_t *>(planarWords.data());

  for (size_t i = 0; i < kTotalGfxBytes; i += 4) {
    const size_t pxBase = i * 2;
    uint8_t p0 = 0, p1 = 0, p2 = 0, p3 = 0;
    for (int bit = 0; bit < 8; ++bit) {
      const int shift = 7 - bit;
      const uint8_t val = padded[pxBase + bit];
      p1 |= ((val >> 0) & 1) << shift;
      p0 |= ((val >> 1) & 1) << shift;
      p3 |= ((val >> 2) & 1) << shift;
      p2 |= ((val >> 3) & 1) << shift;
    }
    planar[i + 0] = p0;
    planar[i + 1] = p1;
    planar[i + 2] = p2;
    planar[i + 3] = p3;
  }

  constexpr size_t kBankWords = 0x200000 / sizeof(uint64_t);
  for (size_t wOff = 0; wOff < planarWords.size(); wOff += kBankWords) {
    shuffle64(planarWords.data() + wOff, kBankWords);
  }

  struct ChipMap {
    const char *suffix;
    size_t baseOffset;
  };
  static constexpr ChipMap kChips[] = {
      {".13", 0x0000000}, {".15", 0x0000002}, {".17", 0x0000004},
      {".19", 0x0000006}, {".14", 0x1000000}, {".16", 0x1000002},
      {".18", 0x1000004}, {".20", 0x1000006},
  };

  constexpr size_t kChipBytes = 0x400000;
  std::vector<uint8_t> chipBuf(kChipBytes, 0);

  for (const auto &chip : kChips) {
    size_t dst = 0;
    for (size_t src = chip.baseOffset; src < chip.baseOffset + 0x1000000;
         src += 8) {
      chipBuf[dst + 0] = planar[src + 1];
      chipBuf[dst + 1] = planar[src + 0];
      dst += 2;
    }
    writeFile(outDir / (romName + chip.suffix), chipBuf.data(), kChipBytes);
  }
}

struct WavSpec {
  bool isBgm = false;
  std::string symbol;
  fs::path wavPath;
};

struct PackedSound {
  bool isBgm = false;
  std::string symbol;
  uint8_t id = 0; // 1..127
  uint16_t bank = 0x8000;
  uint16_t startAddr = 0;
  uint16_t endAddr = 0;
  uint16_t loopLen = 0;
  uint16_t pitch = 0x1000;
  uint8_t chunkCount = 1;
  uint16_t lastEndAddr = 0;
  uint16_t lastTicks = 0;
};

std::vector<uint8_t> decodeWavToQSoundPcm(const fs::path &wavPath, bool isBgm) {
  const auto data = readFile(wavPath);
  if (data.size() < 44 || std::memcmp(data.data(), "RIFF", 4) != 0 ||
      std::memcmp(data.data() + 8, "WAVE", 4) != 0) {
    throw std::runtime_error("Invalid WAV file: " + wavPath.string());
  }

  auto readLE16 = [&](size_t off) -> uint16_t {
    return static_cast<uint16_t>(data[off]) |
           (static_cast<uint16_t>(data[off + 1]) << 8);
  };
  auto readLE32 = [&](size_t off) -> uint32_t {
    return static_cast<uint32_t>(data[off]) |
           (static_cast<uint32_t>(data[off + 1]) << 8) |
           (static_cast<uint32_t>(data[off + 2]) << 16) |
           (static_cast<uint32_t>(data[off + 3]) << 24);
  };

  uint16_t audioFormat = 0;
  uint16_t numChannels = 0;
  uint32_t sampleRate = 0;
  uint16_t bitsPerSample = 0;
  size_t pcmOffset = 0;
  size_t pcmSize = 0;

  size_t pos = 12;
  while (pos + 8 <= data.size()) {
    const uint32_t chunkSize = readLE32(pos + 4);
    if (std::memcmp(data.data() + pos, "fmt ", 4) == 0 && chunkSize >= 16) {
      audioFormat = readLE16(pos + 8);
      numChannels = readLE16(pos + 10);
      sampleRate = readLE32(pos + 12);
      bitsPerSample = readLE16(pos + 22);
    } else if (std::memcmp(data.data() + pos, "data", 4) == 0) {
      pcmOffset = pos + 8;
      pcmSize = std::min<size_t>(chunkSize, data.size() - pcmOffset);
      break;
    }
    pos += 8 + chunkSize + (chunkSize & 1u);
  }

  if (audioFormat != 1 || (bitsPerSample != 8 && bitsPerSample != 16) ||
      numChannels == 0 || sampleRate == 0 || pcmOffset == 0 || pcmSize == 0) {
    throw std::runtime_error(
        "Unsupported WAV format (need uncompressed 8/16-bit PCM): " +
        wavPath.string());
  }

  const size_t bytesPerSample = bitsPerSample / 8;
  const size_t frameStride = bytesPerSample * numChannels;
  const size_t srcFrames = pcmSize / frameStride;

  std::vector<int8_t> mono(srcFrames);
  for (size_t i = 0; i < srcFrames; ++i) {
    int32_t sum = 0;
    for (uint16_t ch = 0; ch < numChannels; ++ch) {
      const size_t off = pcmOffset + i * frameStride + ch * bytesPerSample;
      if (bitsPerSample == 8) {
        sum += static_cast<int32_t>(data[off]) - 128;
      } else {
        const int16_t s16 = static_cast<int16_t>(readLE16(off));
        sum += (s16 >> 8);
      }
    }
    int32_t avg = sum / numChannels;
    avg = std::max<int32_t>(-127, std::min<int32_t>(127, avg));
    mono[i] = static_cast<int8_t>(avg);
  }

  // Resample to QSound native 24038 Hz:
  //   - SFX capped at 0x7fe0 samples (~1.36s, single bank)
  //   - BGM capped at 32 * 31937 samples (~42.5s across 32 banks of hb.11)
  constexpr double kQSoundRate = 24038.0;
  constexpr size_t kMaxSfxFrames = 0x7fe0u;
  constexpr size_t kMaxBgmFrames = 32u * 31937u;
  const size_t maxFrames = isBgm ? kMaxBgmFrames : kMaxSfxFrames;

  size_t dstFrames =
      static_cast<size_t>((static_cast<double>(srcFrames) * kQSoundRate) /
                          static_cast<double>(sampleRate));
  dstFrames = std::max<size_t>(1, std::min<size_t>(dstFrames, maxFrames));

  std::vector<uint8_t> out(dstFrames);
  for (size_t i = 0; i < dstFrames; ++i) {
    const size_t srcIdx = std::min<size_t>(
        srcFrames - 1,
        static_cast<size_t>((static_cast<double>(i) * sampleRate) /
                            kQSoundRate));
    out[i] = static_cast<uint8_t>(mono[srcIdx]);
  }
  return out;
}

void writeAssetsHeader(const fs::path &headerPath,
                       const std::vector<PackedSheet> &sheets,
                       const std::vector<PackedSound> &sounds) {
  std::ofstream out(headerPath);
  if (!out.is_open()) {
    throw std::runtime_error("Failed to open assets header: " +
                             headerPath.string());
  }

  out << "#pragma once\n#include <stdint.h>\n\n";
  for (const auto &s : sheets) {
    out << "#define " << s.symbol << "_FIRST_TILE " << s.firstTile << "u\n";
    out << "#define " << s.symbol << "_TILE_COUNT " << s.tileCount << "u\n";
    out << "static const uint16_t " << s.symbol << "_PALETTE[16] = {";
    for (int i = 0; i < 16; ++i) {
      if (i > 0)
        out << ", ";
      char hexBuf[16];
      std::snprintf(hexBuf, sizeof(hexBuf), "0x%04Xu", s.palette[i]);
      out << hexBuf;
    }
    out << "};\n";
    out << "#if defined(CPS2_SDK_PALETTE_H)\n";
    out << "static constexpr cps2::TileSheet " << s.symbol << "_SHEET{"
        << s.symbol << "_FIRST_TILE, " << s.symbol << "_TILE_COUNT, "
        << s.symbol << "_PALETTE};\n";
    out << "#elif defined(CPS2_SDK_CPS2_C_MODULE)\n";
    out << "static const Cps2TileSheet " << s.symbol << "_SHEET = {" << s.symbol
        << "_FIRST_TILE, " << s.symbol << "_TILE_COUNT, " << s.symbol
        << "_PALETTE};\n";
    out << "#endif\n\n";
  }
  for (const auto &snd : sounds) {
    const uint16_t cmdId = snd.isBgm ? (0x80u | snd.id) : snd.id;
    out << "#define " << snd.symbol << "_ID 0x" << std::hex << std::uppercase
        << cmdId << std::dec << "u\n";
  }
}

} // namespace

int main(int argc, char **argv) {
  try {
    fs::path progPath;
    fs::path elfPath;
    fs::path outDir;
    fs::path headerOut;
    std::string keyFileArg;
    bool hasCustomKey = false;
    u32 customK0 = 0;
    u32 customK1 = 0;
    std::string romName = "hb";
    bool doEncrypt = false;
    std::vector<PngSheetSpec> pngSpecs;
    std::vector<WavSpec> wavSpecs;
    fs::path z80Path;
    fs::path qsoundPath;

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--prog" && i + 1 < argc) {
        progPath = argv[++i];
      } else if (arg == "--elf" && i + 1 < argc) {
        elfPath = argv[++i];
      } else if (arg == "--out-dir" && i + 1 < argc) {
        outDir = argv[++i];
      } else if (arg == "--name" && i + 1 < argc) {
        romName = argv[++i];
      } else if (arg == "--header" && i + 1 < argc) {
        headerOut = argv[++i];
      } else if (arg == "--encrypt") {
        doEncrypt = true;
      } else if ((arg == "--png" || arg == "--sprite") && i + 3 < argc) {
        PngSheetSpec spec;
        spec.isSprite = (arg == "--sprite");
        spec.symbol = argv[++i];
        spec.pngPath = argv[++i];
        spec.tileSize = std::stoi(argv[++i]);
        pngSpecs.push_back(std::move(spec));
      } else if (arg == "--key-file" && i + 1 < argc) {
        keyFileArg = argv[++i];
        doEncrypt = true;
      } else if (arg == "--key" && i + 2 < argc) {
        hasCustomKey = true;
        customK0 = static_cast<u32>(std::stoul(argv[++i], nullptr, 16));
        customK1 = static_cast<u32>(std::stoul(argv[++i], nullptr, 16));
        doEncrypt = true;
      } else if (arg == "--z80" && i + 1 < argc) {
        z80Path = argv[++i];
      } else if (arg == "--qsound" && i + 1 < argc) {
        qsoundPath = argv[++i];
      } else if ((arg == "--sfx" || arg == "--bgm") && i + 2 < argc) {
        WavSpec ws;
        ws.isBgm = (arg == "--bgm");
        ws.symbol = argv[++i];
        ws.wavPath = argv[++i];
        wavSpecs.push_back(std::move(ws));
      }
    }

    if (progPath.empty() || outDir.empty()) {
      std::cerr << "Usage: cps2_packer --prog <game.bin> --out-dir <dir> "
                   "[--elf <game.elf>] [--encrypt] [--name <hb>] "
                   "[--header <assets.h>] [--png <SYM> <file.png> <8|16>]\n";
      return 1;
    }

    fs::create_directories(outDir);

    auto progBytes = readFile(progPath);
    if (progBytes.size() & 1) {
      progBytes.push_back(0x00);
    }

    if (doEncrypt) {
      if (elfPath.empty()) {
        throw std::runtime_error("--encrypt requires --elf <game.elf>");
      }
      const auto isCode = buildElfCodeMask(elfPath, progBytes.size());

      CPS2::KeyFile kf{};
      std::array<uint8_t, 20> keyFileBytes{};

      if (!keyFileArg.empty()) {
        const auto rawKey = readFile(keyFileArg);
        if (rawKey.size() < 20) {
          throw std::runtime_error("Key file must be at least 20 bytes: " +
                                   keyFileArg);
        }
        std::memcpy(keyFileBytes.data(), rawKey.data(), 20);
        kf = CPS2::CPS2Crypt::parseKey(keyFileArg);
      } else {
        const uint32_t k0 = hasCustomKey ? customK0 : 0x12345678u;
        const uint32_t k1 = hasCustomKey ? customK1 : 0x9abcdef0u;

        std::array<uint16_t, 10> keyWords{};
        keyWords[0] = static_cast<uint16_t>((k0 >> 16) & 0xffffu);
        keyWords[1] = static_cast<uint16_t>(k0 & 0xffffu);
        keyWords[2] = static_cast<uint16_t>((k1 >> 16) & 0xffffu);
        keyWords[3] = static_cast<uint16_t>(k1 & 0xffffu);
        keyWords[9] = 0x0000;

        keyFileBytes = encodeKeyFile(keyWords);
        kf.masterKey[0] = k0;
        kf.masterKey[1] = k1;
        kf.lowerLimit = 0;
        kf.upperLimit = 0x800000;
      }

      writeFile(outDir / (romName + ".key"), keyFileBytes.data(),
                keyFileBytes.size());

      std::vector<uint8_t> encryptedProg;
      CPS2::CPS2Crypt::encrypt(progBytes, encryptedProg, kf, isCode);
      std::vector<uint8_t> verifyDec;
      CPS2::CPS2Crypt::decrypt(encryptedProg, verifyDec, kf);
      for (size_t w = 0; w < isCode.size(); ++w) {
        if (isCode[w] != 0 && (verifyDec[w * 2] != progBytes[w * 2] ||
                               verifyDec[w * 2 + 1] != progBytes[w * 2 + 1])) {
          throw std::runtime_error("CPS2Crypt round-trip mismatch at word " +
                                   std::to_string(w));
        }
      }
      progBytes = std::move(encryptedProg);
    }

    // Pack --sprite sheets at 0 (OBJ space), then place Scroll 1/2/3 font and
    // --png background sheets starting at tileGfxBase (0x1000000)
    constexpr size_t kScrollGfxBase = 0x1000000;
    std::vector<uint8_t> decodedGfx;
    std::vector<PackedSheet> packedSheets;

    for (const auto &spec : pngSpecs) {
      if (spec.isSprite) {
        packedSheets.push_back(importPngSheet(spec, decodedGfx, 0));
      }
    }

    if (decodedGfx.size() < kScrollGfxBase) {
      decodedGfx.resize(kScrollGfxBase, 0x0f);
    }
    const auto fontTiles = buildBuiltinFontRegion();
    decodedGfx.insert(decodedGfx.end(), fontTiles.begin(), fontTiles.end());

    for (const auto &spec : pngSpecs) {
      if (!spec.isSprite) {
        packedSheets.push_back(
            importPngSheet(spec, decodedGfx, kScrollGfxBase));
      }
    }

    std::vector<PackedSound> packedSounds;
    std::vector<uint8_t> qsBytes;
    if (!qsoundPath.empty()) {
      qsBytes = readFile(qsoundPath);
    }

    constexpr size_t kStreamChunkSamples =
        31937u; // 335 Z80 IRQs @ 252 Hz (~1.33s)

    for (size_t idx = 0; idx < wavSpecs.size(); ++idx) {
      if (idx >= 127) {
        throw std::runtime_error("Maximum 127 WAV samples supported");
      }
      const auto &ws = wavSpecs[idx];
      const auto pcm = decodeWavToQSoundPcm(ws.wavPath, ws.isBgm);

      if (ws.isBgm && pcm.size() > kStreamChunkSamples) {
        // Multi-bank BGM streaming: align to next 64 KB bank boundary
        if ((qsBytes.size() & 0xffffu) != 0) {
          const size_t nextBankOffset = (qsBytes.size() + 0x10000u) & ~0xffffu;
          qsBytes.resize(nextBankOffset, 0x00);
        }

        const size_t numChunks =
            (pcm.size() + kStreamChunkSamples - 1u) / kStreamChunkSamples;
        if (numChunks > 32u ||
            qsBytes.size() + numChunks * 0x10000u > 0x200000u) {
          throw std::runtime_error("Streaming BGM exceeds 2 MB QSound ROM");
        }

        const uint16_t firstBank =
            static_cast<uint16_t>(0x8000u | (qsBytes.size() >> 16));
        uint16_t lastEndAddr = 0;
        uint16_t lastTicks = 1;

        for (size_t c = 0; c < numChunks; ++c) {
          const size_t chunkStart = c * kStreamChunkSamples;
          const size_t chunkLen =
              std::min<size_t>(kStreamChunkSamples, pcm.size() - chunkStart);

          // Place chunk at offset 0x0000 of its own 64 KB bank
          qsBytes.insert(qsBytes.end(), pcm.begin() + chunkStart,
                         pcm.begin() + chunkStart + chunkLen);

          for (size_t pad = 0; pad < 16; ++pad) {
            const size_t nextIdx = (chunkStart + chunkLen + pad) % pcm.size();
            qsBytes.push_back(pcm[nextIdx]);
          }

          if (c + 1 == numChunks) {
            lastEndAddr = static_cast<uint16_t>(chunkLen);
            // 95.3333 samples per 252 Hz Z80 IRQ (286 / 3)
            const uint32_t ticks =
                static_cast<uint32_t>((chunkLen * 3u + 143u) / 286u);
            lastTicks = static_cast<uint16_t>(std::max<uint32_t>(1u, ticks));
          }

          // Pad bank to full 64 KB (0x10000) so the next chunk lands on bank +
          // 1
          const size_t rem = qsBytes.size() & 0xffffu;
          if (rem != 0) {
            qsBytes.resize(qsBytes.size() + (0x10000u - rem), 0x00);
          }
        }

        PackedSound ps;
        ps.isBgm = true;
        ps.symbol = ws.symbol;
        ps.id = static_cast<uint8_t>(idx + 1);
        ps.bank = firstBank;
        ps.startAddr = 0x0000u;
        ps.endAddr = static_cast<uint16_t>(kStreamChunkSamples);
        ps.loopLen = 0x0001u;
        ps.pitch = 0x1000u;
        ps.chunkCount = static_cast<uint8_t>(numChunks);
        ps.lastEndAddr = lastEndAddr;
        ps.lastTicks = lastTicks;
        packedSounds.push_back(ps);
      } else {
        // Single-bank SFX or short BGM loop
        while ((qsBytes.size() & 0xffu) != 0) {
          qsBytes.push_back(0x00);
        }

        const size_t totalLen = pcm.size() + 16;
        if ((qsBytes.size() & 0xffffu) + totalLen > 0x8000u) {
          const size_t nextBankOffset = (qsBytes.size() + 0x10000u) & ~0xffffu;
          qsBytes.resize(nextBankOffset, 0x00);
        }
        if (qsBytes.size() + totalLen > 0x200000u) {
          throw std::runtime_error("QSound sample ROM (2 MB) exceeded");
        }

        PackedSound ps;
        ps.isBgm = ws.isBgm;
        ps.symbol = ws.symbol;
        ps.id = static_cast<uint8_t>(idx + 1);
        ps.bank = static_cast<uint16_t>(0x8000u | (qsBytes.size() >> 16));
        ps.startAddr = static_cast<uint16_t>(qsBytes.size() & 0x7fffu);
        qsBytes.insert(qsBytes.end(), pcm.begin(), pcm.end());
        qsBytes.insert(qsBytes.end(), 16, 0x00);
        ps.endAddr = static_cast<uint16_t>((qsBytes.size() - 1) & 0x7fffu);
        ps.loopLen = ws.isBgm ? static_cast<uint16_t>(ps.endAddr - ps.startAddr)
                              : 0x0001u;
        ps.pitch = 0x1000u;
        ps.chunkCount = 1u;
        ps.lastEndAddr = ps.endAddr;
        ps.lastTicks = 0u;
        packedSounds.push_back(ps);
      }
    }

    if (!headerOut.empty()) {
      writeAssetsHeader(headerOut, packedSheets, packedSounds);
    }

    if (!z80Path.empty() || !packedSounds.empty()) {
      constexpr size_t kZ80ChipBytes = 0x20000; // 128 KB per audiocpu chip
      std::vector<uint8_t> z80Bytes;
      if (!z80Path.empty()) {
        z80Bytes = readFile(z80Path);
      } else {
        z80Bytes.assign(std::begin(kDefaultZ80Driver),
                        std::end(kDefaultZ80Driver));
      }

      const size_t totalZ80 = (z80Bytes.size() > kZ80ChipBytes)
                                  ? (kZ80ChipBytes * 2)
                                  : kZ80ChipBytes;
      z80Bytes.resize(totalZ80, 0x00);

      // Patch WAV sample descriptors into Z80 sample table (0x0400 + id * 16)
      for (const auto &ps : packedSounds) {
        const size_t off = 0x0400u + static_cast<size_t>(ps.id) * 16u;
        z80Bytes[off + 0] = static_cast<uint8_t>((ps.startAddr >> 8) & 0xffu);
        z80Bytes[off + 1] = static_cast<uint8_t>(ps.startAddr & 0xffu);
        z80Bytes[off + 2] = static_cast<uint8_t>((ps.endAddr >> 8) & 0xffu);
        z80Bytes[off + 3] = static_cast<uint8_t>(ps.endAddr & 0xffu);
        z80Bytes[off + 4] = static_cast<uint8_t>((ps.loopLen >> 8) & 0xffu);
        z80Bytes[off + 5] = static_cast<uint8_t>(ps.loopLen & 0xffu);
        z80Bytes[off + 6] = static_cast<uint8_t>((ps.pitch >> 8) & 0xffu);
        z80Bytes[off + 7] = static_cast<uint8_t>(ps.pitch & 0xffu);
        z80Bytes[off + 8] = static_cast<uint8_t>((ps.bank >> 8) & 0xffu);
        z80Bytes[off + 9] = static_cast<uint8_t>(ps.bank & 0xffu);
        z80Bytes[off + 10] = ps.chunkCount;
        z80Bytes[off + 11] =
            static_cast<uint8_t>((ps.lastEndAddr >> 8) & 0xffu);
        z80Bytes[off + 12] = static_cast<uint8_t>(ps.lastEndAddr & 0xffu);
        z80Bytes[off + 13] = static_cast<uint8_t>((ps.lastTicks >> 8) & 0xffu);
        z80Bytes[off + 14] = static_cast<uint8_t>(ps.lastTicks & 0xffu);
      }

      writeFile(outDir / (romName + ".01"), z80Bytes.data(), kZ80ChipBytes);
      if (totalZ80 > kZ80ChipBytes) {
        writeFile(outDir / (romName + ".02"), z80Bytes.data() + kZ80ChipBytes,
                  kZ80ChipBytes);
      }
    }

    if (!qsBytes.empty()) {
      constexpr size_t kQSoundChipBytes =
          0x200000; // 2 MB per qsound sample chip
      const size_t totalQs = (qsBytes.size() > kQSoundChipBytes)
                                 ? (kQSoundChipBytes * 2)
                                 : kQSoundChipBytes;
      qsBytes.resize(totalQs, 0x00);
      writeFile(outDir / (romName + ".11"), qsBytes.data(), kQSoundChipBytes);
      if (totalQs > kQSoundChipBytes) {
        writeFile(outDir / (romName + ".12"), qsBytes.data() + kQSoundChipBytes,
                  kQSoundChipBytes);
      }
    }

    // byte-swap 68000 program ROM for CPS2 .03 slot
    std::vector<uint8_t> swappedProg(progBytes.size());
    for (size_t i = 0; i < progBytes.size(); i += 2) {
      swappedProg[i + 0] = progBytes[i + 1];
      swappedProg[i + 1] = progBytes[i + 0];
    }
    writeFile(outDir / (romName + ".03"), swappedProg.data(),
              swappedProg.size());

    writeInterleavedGfxRoms(decodedGfx, outDir, romName);

    std::cout << "Packed CPS2 ROM set '" << romName << "' into " << outDir
              << " (" << swappedProg.size() << "B prog"
              << (doEncrypt ? ", encrypted" : "") << ")\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "cps2_packer error: " << ex.what() << "\n";
    return 1;
  }
}
