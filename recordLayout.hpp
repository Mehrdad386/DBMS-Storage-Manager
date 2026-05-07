#include <iostream>
#include <string>
#include <vector>
#include <cstddef>
#include <cstring>
#include <bit>
#include <cstdint>

class recordLayout
{

public:
    struct record
    {
        int id;
        std::string name;
        float grade;
        bool isActive;
    };

    std::vector<std::byte> serializeRecord(record r)
    {
        std::vector<std::byte> result;
        result.reserve(30); // Reserve 30 bytes

        // 1. Serialize id (4 bytes, little-endian)
        for (int i = 0; i < 4; i++)
        {
            uint8_t byte = (r.id >> (i * 8)) & 0xFF; // Extract bytes in little-endian order
            result.push_back(static_cast<std::byte>(byte));
        }

        // 2. Serialize name (exactly 20 bytes, pad with spaces)
        if (r.name.length() > 20)
        {
            r.name = r.name.substr(0, 20); // Truncate if too long
        }
        if (r.name.length() < 20)
        {
            r.name.append(20 - r.name.length(), ' '); // Pad with spaces
        }

        for (char c : r.name)
        {
            result.push_back(static_cast<std::byte>(c));
        }

        // 3. Serialize grade (4 bytes, little-endian float)
        // Convert float to bytes using memcpy
        uint8_t *gradeBytes = reinterpret_cast<uint8_t *>(&r.grade);
        for (int i = 0; i < 4; i++)
        {
            result.push_back(static_cast<std::byte>(gradeBytes[i]));
        }

        // 4. Serialize is_active (1 byte)
        result.push_back(static_cast<std::byte>(r.isActive ? 1 : 0));

        // 5. Serialize null_bitmap (1 byte, always 0)
        result.push_back(static_cast<std::byte>(0));

        return result;
    }

    record deserialize(std::vector<std::byte> &data)
    {

        if (data.size() < 30)
        {
            throw std::runtime_error("Data too small: expected 30 bytes");
        }

        record result;
        size_t offset = 0;

        // 1. Extract id (first 4 bytes, little-endian)
        int id = 0;
        for (int i = 0; i < 4; i++)
        {
            id |= (static_cast<int>(data[offset + i]) << (i * 8));
        }
        result.id = id;
        offset += 4;

        // 2. Extract name (next 20 bytes)
        std::string name;
        for (int i = 0; i < 20; i++)
        {
            char c = static_cast<char>(data[offset + i]);
            name += c;
        }
        // Remove trailing spaces
        while (!name.empty() && name.back() == ' ')
        {
            name.pop_back();
        }
        result.name = name;
        offset += 20;

        // 3. Extract grade (next 4 bytes as float)
        float grade;
        memcpy(&grade, &data[offset], 4);
        result.grade = grade;
        offset += 4;

        // 4. Extract isActive (next 1 byte)
        result.isActive = (static_cast<int>(data[offset]) != 0);
        offset += 1;

        // 5. Skip null_bitmap (last 1 byte, always 0)
        // offset += 1;  // Not needed for result

        return result;
    }

private:
};