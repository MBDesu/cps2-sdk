#pragma once
#ifndef CPS2_CPS2Crypt_MODULE_H
#define CPS2_CPS2Crypt_MODULE_H

#include "globals.h"
#include <array>
#include <filesystem>
#include <vector>

namespace CPS2 {

struct KeyFile {
  std::array<u32, 2> masterKey{};
  u32 lowerLimit = 0;
  u32 upperLimit = 0;
};

class CPS2Crypt {
public:
  static KeyFile parseKey(const std::filesystem::path &keyPath);
  static void decrypt(const std::vector<u8> &raw, std::vector<u8> &decrypted,
                      const KeyFile &key);
  static void encrypt(const std::vector<u8> &plain, std::vector<u8> &encrypted,
                      const KeyFile &key, const std::vector<u8> &isCodeWord);
  static std::array<u8, 20> encodeKey(u32 k0, u32 k1,
                                      u32 upperByteLimit = 0x100000);
};

} // namespace CPS2

#endif // CPS2_CPS2Crypt_MODULE_H
