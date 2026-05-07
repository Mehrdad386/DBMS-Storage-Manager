#include <iostream>
#include <string>
#include <vector>
#include <cstddef>
#include <cstring>
#include <bit>
#include <cstdint>

class recordLayout {

public:
    std::vector<std::byte> serializeRecord(int id, std::string name, float grade, bool isActive) {
        std::vector<std::byte> result;
        result.reserve(30);  // Reserve 30 bytes
        
        // 1. Serialize id (4 bytes, little-endian)
        for (int i = 0; i < 4; i++) {
            uint8_t byte = (id >> (i * 8)) & 0xFF;  // Extract bytes in little-endian order
            result.push_back(static_cast<std::byte>(byte));
        }
        
        // 2. Serialize name (exactly 20 bytes, pad with spaces)
        if (name.length() > 20) {
            name = name.substr(0, 20);  // Truncate if too long
        }
        if (name.length() < 20) {
            name.append(20 - name.length(), ' ');  // Pad with spaces
        }
        
        for (char c : name) {
            result.push_back(static_cast<std::byte>(c));
        }
        
        // 3. Serialize grade (4 bytes, little-endian float)
        // Convert float to bytes using memcpy
        uint8_t* gradeBytes = reinterpret_cast<uint8_t*>(&grade);
        for (int i = 0; i < 4; i++) {
            result.push_back(static_cast<std::byte>(gradeBytes[i]));
        }
        
        // 4. Serialize is_active (1 byte)
        result.push_back(static_cast<std::byte>(isActive ? 1 : 0));
        
        // 5. Serialize null_bitmap (1 byte, always 0)
        result.push_back(static_cast<std::byte>(0));
        
        return result;
    }
private:


};