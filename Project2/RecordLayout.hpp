#ifndef RECORDLAYOUT
#define RECORDLAYOUT

#include <iostream>
#include "Course.hpp"
#include <vector>
#include <cstddef>
#include <cstring>
#include <bit>
#include <cstdint>

class RecordLayout
{
public:
    std::vector<std::byte> serializeRecord(const Course &r)
    {
        std::vector<std::byte> result;
        result.reserve(28);

        for (int i = 0; i < 4; i++)
        {
            uint8_t byte = (r.course_id >> (i * 8)) & 0xFF;
            result.push_back(static_cast<std::byte>(byte));
        }

        std::string title = r.title;

        if (title.length() > 20)
        {
            title = title.substr(0, 20);
        }

        if (title.length() < 20)
        {
            title.append(20 - title.length(), ' ');
        }

        for (char c : title)
        {
            result.push_back(static_cast<std::byte>(c));
        }

        for (int i = 0; i < 4; i++)
        {
            uint8_t byte = (r.credits >> (i * 8)) & 0xFF;
            result.push_back(static_cast<std::byte>(byte));
        }

        return result;
    }

    Course deserialize(const std::vector<std::byte> &data)
    {
        if (data.size() < 28)
        {
            throw std::runtime_error("Data too small: expected 28 bytes");
        }

        Course result;
        size_t offset = 0;

        int id = 0;
        for (int i = 0; i < 4; i++)
        {
            id |= (std::to_integer<int>(data[offset + i]) << (i * 8));
        }
        result.course_id = id;
        offset += 4;

        std::string title;
        for (int i = 0; i < 20; i++)
        {
            char c = static_cast<char>(std::to_integer<unsigned char>(data[offset + i]));
            title += c;
        }

        while (!title.empty() && title.back() == ' ')
        {
            title.pop_back();
        }

        result.title = title;
        offset += 20;

        int credits = 0;
        for (int i = 0; i < 4; i++)
        {
            credits |= (std::to_integer<int>(data[offset + i]) << (i * 8));
        }

        result.credits = credits;

        return result;
    }

private:
};

#endif