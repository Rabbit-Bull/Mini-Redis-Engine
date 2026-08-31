#include "mini_redis/fen_compressor.hpp"
#include <cctype>
#include <cstdint>
#include <sstream>
#include <vector>

namespace mini_redis {

namespace {

uint8_t piece_to_nibble(char p) {
    switch (p) {
        case 'P': return 1;
        case 'N': return 2;
        case 'B': return 3;
        case 'R': return 4;
        case 'Q': return 5;
        case 'K': return 6;
        case 'p': return 7;
        case 'n': return 8;
        case 'b': return 9;
        case 'r': return 10;
        case 'q': return 11;
        case 'k': return 12;
        default: return 0;
    }
}

char nibble_to_piece(uint8_t n) {
    switch (n) {
        case 1: return 'P';
        case 2: return 'N';
        case 3: return 'B';
        case 4: return 'R';
        case 5: return 'Q';
        case 6: return 'K';
        case 7: return 'p';
        case 8: return 'n';
        case 9: return 'b';
        case 10: return 'r';
        case 11: return 'q';
        case 12: return 'k';
        default: return '\0';
    }
}

std::vector<std::string> split_whitespace(const std::string& str) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

} // anonymous namespace

std::string FenCompressor::compress(const std::string& fen) {
    std::vector<std::string> fields = split_whitespace(fen);
    if (fields.empty()) {
        return "";
    }

    // 1. Board state (64 squares -> 32 bytes)
    std::vector<uint8_t> squares(64, 0);
    size_t sq_idx = 0;
    const std::string& board_str = fields[0];

    for (char c : board_str) {
        if (c == '/') {
            continue;
        } else if (c >= '1' && c <= '8') {
            int empty_count = c - '0';
            sq_idx += empty_count;
        } else {
            uint8_t nibble = piece_to_nibble(c);
            if (nibble != 0 && sq_idx < 64) {
                squares[sq_idx++] = nibble;
            }
        }
    }

    if (sq_idx != 64) {
        return ""; // Invalid board specification
    }

    std::string result;
    result.reserve(38);

    // Pack 64 nibbles into 32 bytes
    for (size_t i = 0; i < 32; ++i) {
        uint8_t high = squares[2 * i] & 0x0F;
        uint8_t low = squares[2 * i + 1] & 0x0F;
        uint8_t byte = static_cast<uint8_t>((high << 4) | low);
        result.push_back(static_cast<char>(byte));
    }

    // Metadata fields with standard defaults
    char active_color = (fields.size() > 1) ? fields[1][0] : 'w';
    std::string castling = (fields.size() > 2) ? fields[2] : "-";
    std::string en_passant = (fields.size() > 3) ? fields[3] : "-";
    uint16_t halfmove = (fields.size() > 4) ? static_cast<uint16_t>(std::stoi(fields[4])) : 0;
    uint16_t fullmove = (fields.size() > 5) ? static_cast<uint16_t>(std::stoi(fields[5])) : 1;

    // Metadata byte 1 (Active color + Castling)
    uint8_t meta1 = 0;
    if (active_color == 'b') meta1 |= 0x80;
    if (castling.find('K') != std::string::npos) meta1 |= 0x40;
    if (castling.find('Q') != std::string::npos) meta1 |= 0x20;
    if (castling.find('k') != std::string::npos) meta1 |= 0x10;
    if (castling.find('q') != std::string::npos) meta1 |= 0x08;
    result.push_back(static_cast<char>(meta1));

    // Metadata byte 2 (En passant square index: 0..63 or 255 for '-')
    uint8_t ep_byte = 255;
    if (en_passant != "-" && en_passant.length() >= 2) {
        int file = en_passant[0] - 'a';
        int rank = en_passant[1] - '1';
        if (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
            ep_byte = static_cast<uint8_t>(rank * 8 + file);
        }
    }
    result.push_back(static_cast<char>(ep_byte));

    // Metadata bytes 3-4 (Halfmove clock)
    result.push_back(static_cast<char>((halfmove >> 8) & 0xFF));
    result.push_back(static_cast<char>(halfmove & 0xFF));

    // Metadata bytes 5-6 (Fullmove number)
    result.push_back(static_cast<char>((fullmove >> 8) & 0xFF));
    result.push_back(static_cast<char>(fullmove & 0xFF));

    return result;
}

std::string FenCompressor::decompress(const std::string& compressed) {
    if (compressed.size() < 38) {
        return ""; // Invalid binary payload
    }

    // 1. Unpack 32 bytes into 64 nibble board squares
    std::vector<uint8_t> squares(64, 0);
    for (size_t i = 0; i < 32; ++i) {
        uint8_t byte = static_cast<uint8_t>(compressed[i]);
        squares[2 * i] = (byte >> 4) & 0x0F;
        squares[2 * i + 1] = byte & 0x0F;
    }

    // 2. Reconstruct board ranks (Rank 8 to Rank 1)
    std::string board_str;
    for (int rank = 7; rank >= 0; --rank) {
        int empty_count = 0;
        for (int file = 0; file < 8; ++file) {
            int sq_idx = (7 - rank) * 8 + file;
            uint8_t nibble = squares[sq_idx];
            if (nibble == 0) {
                empty_count++;
            } else {
                if (empty_count > 0) {
                    board_str += std::to_string(empty_count);
                    empty_count = 0;
                }
                board_str.push_back(nibble_to_piece(nibble));
            }
        }
        if (empty_count > 0) {
            board_str += std::to_string(empty_count);
        }
        if (rank > 0) {
            board_str.push_back('/');
        }
    }

    // 3. Unpack Metadata byte 1
    uint8_t meta1 = static_cast<uint8_t>(compressed[32]);
    std::string active_color = (meta1 & 0x80) ? "b" : "w";

    std::string castling;
    if (meta1 & 0x40) castling += 'K';
    if (meta1 & 0x20) castling += 'Q';
    if (meta1 & 0x10) castling += 'k';
    if (meta1 & 0x08) castling += 'q';
    if (castling.empty()) castling = "-";

    // 4. Unpack Metadata byte 2 (En passant)
    uint8_t ep_byte = static_cast<uint8_t>(compressed[33]);
    std::string en_passant = "-";
    if (ep_byte < 64) {
        char file = static_cast<char>('a' + (ep_byte % 8));
        char rank = static_cast<char>('1' + (ep_byte / 8));
        en_passant = std::string(1, file) + std::string(1, rank);
    }

    // 5. Unpack Halfmove clock
    uint16_t halfmove = (static_cast<uint8_t>(compressed[34]) << 8) | static_cast<uint8_t>(compressed[35]);

    // 6. Unpack Fullmove number
    uint16_t fullmove = (static_cast<uint8_t>(compressed[36]) << 8) | static_cast<uint8_t>(compressed[37]);

    return board_str + " " + active_color + " " + castling + " " + en_passant + " " + std::to_string(halfmove) + " " + std::to_string(fullmove);
}

} // namespace mini_redis
