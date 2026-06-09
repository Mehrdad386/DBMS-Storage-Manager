#ifndef PAGELAYOUT
#define PAGELAYOUT

#include <vector>
#include <cstdint>
#include <cstddef>
#include <stdexcept>

class PageLayout
{
public:
    std::vector<std::byte> initPage()
    {
        std::vector<std::byte> page(512, static_cast<std::byte>(0));

        int recordCount = 0;
        for (int i = 0; i < 4; i++)
        {
            uint8_t byte = (recordCount >> (i * 8)) & 0xFF;
            page[i] = static_cast<std::byte>(byte);
        }

        return page;
    }

    std::vector<std::byte> addRecordToPage(std::vector<std::byte> &page, const std::vector<std::byte> &recordBytes)
    {
        int count = 0;
        for (int i = 0; i < 4; i++)
        {
            count |= (std::to_integer<int>(page[i]) << (i * 8));
        }

        if (count >= 18)
        {
            throw std::overflow_error("page is full");
        }

        int offset = 4 + (count * 28);

        for (std::byte rb : recordBytes)
        {
            page[offset++] = rb;
        }

        count++;

        for (int i = 0; i < 4; i++)
        {
            uint8_t byte = (count >> (i * 8)) & 0xFF;
            page[i] = static_cast<std::byte>(byte);
        }

        return page;
    }

    std::vector<std::byte> getRecordFromPage(std::vector<std::byte> &page, int slotNumber)
    {
        if (slotNumber < 1 || slotNumber > 18)
        {
            throw std::out_of_range("invalid slot number");
        }

        int offset = 4 + ((slotNumber - 1) * 28);

        std::vector<std::byte> record;
        record.reserve(28);

        for (int i = 0; i < 28; i++)
        {
            record.push_back(page[offset + i]);
        }

        return record;
    }

private:
};

#endif