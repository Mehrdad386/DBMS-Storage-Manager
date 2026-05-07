#include "recordLayout.hpp"

class pageLayout
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

    std::vector<std::byte> addRecordToPage(std::vector<std::byte> &page, std::vector<std::byte> &recordBytes)
    {
        // step 1 : reading current record count
        int count = 0;
        for (int i = 0; i < 4; i++)
        {
            count |= (static_cast<int>(page[i]) << (i * 8));
        }
        // step 2 : to check if page is full
        if (count > 16)
            std::__throw_overflow_error("page is full");
        // step 3 : determine offset
        int offset = 4 + (count * 30);
        // step 4 : copying record bytes to slot offset
        for (std::byte rb : recordBytes)
        {
            page[offset] = rb;
            offset++;
        }
        // step 5 : increment the count by 1
        count++;
        for (int i = 0; i < 4; i++)
        {
            uint8_t byte = (count >> (i * 8)) & 0xFF; // Extract bytes in little-endian order
            page[i] = static_cast<std::byte>(byte);
        }
        // step 6 : return page_bytes
        return page;
    }

private:
};