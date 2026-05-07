#include"recordLayout.hpp"

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




private:

};