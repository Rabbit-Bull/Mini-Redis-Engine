#ifndef MINI_REDIS_FEN_COMPRESSOR_HPP
#define MINI_REDIS_FEN_COMPRESSOR_HPP

#include <string>

namespace mini_redis {

class FenCompressor {
public:
    // Compresses a standard ASCII FEN string into a dense binary buffer (std::string)
    static std::string compress(const std::string& fen);

    // Decompresses a dense binary buffer (std::string) back into a standard ASCII FEN string
    static std::string decompress(const std::string& compressed);
};

} // namespace mini_redis

#endif // MINI_REDIS_FEN_COMPRESSOR_HPP
