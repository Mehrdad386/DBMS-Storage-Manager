#include <vector>
#include <fstream>
#include <cstddef>

class fileManager
{

public:
    void writePage(std::string &fileName, int pageNum, std::vector<std::byte> &pageByte)
    {
        std::fstream file(fileName, std::ios::binary | std::ios::in | std::ios::out);
        if (!file.is_open())
        {
            file.open(fileName, std::ios::binary | std::ios::out);
            file.close();
            file.open(fileName, std::ios::binary | std::ios::in | std::ios::out);
        }
        // Calculate offset
        std::streamoff offset = static_cast<std::streamoff>(pageNum) * 512;

        // Seek to position
        file.seekp(offset, std::ios::beg);

        // Write 512 bytes
        file.write(reinterpret_cast<const char *>(pageByte.data()), pageByte.size());

        // Close file
        file.close();
    }

    std::vector<std::byte> readPage(std::string &fileName, int pageNum)
    {
        std::fstream file(fileName, std::ios::binary | std::ios::in);

        // Calculate offset
        std::streamoff offset = static_cast<std::streamoff>(pageNum) * 512;

        // Seek to position
        file.seekg(offset, std::ios::beg);

        // Read exactly 512 bytes
        std::vector<std::byte> pageBytes(512);
        file.read(reinterpret_cast<char *>(pageBytes.data()), 512);

        // Close file
        file.close();

        return pageBytes;
    }

private:
};