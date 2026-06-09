#ifndef INDEXMANAGER
#define INDEXMANAGER

#include <vector>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <cstdint>

class IndexManager
{

public:
    void insertIndex(int studentId, int pageId, int slot)
    {

        int lastPageNum = getLastPageNumber();

        if (lastPageNum == -1)
        {

            std::vector<std::byte> newPage(PAGE_SIZE, static_cast<std::byte>(0));
            setEntryCount(newPage, 0);
            writeEntry(newPage, 0, studentId, pageId, slot);
            setEntryCount(newPage, 1);
            writePage(0, newPage);
            std::cout << "Created new index page 0 with first entry" << std::endl;
            return;
        }

        std::vector<std::byte> lastPage = readPage(lastPageNum);
        int currentCount = getEntryCount(lastPage);

        if (currentCount < ENTRIES_PER_PAGE)
        {

            writeEntry(lastPage, currentCount, studentId, pageId, slot);
            setEntryCount(lastPage, currentCount + 1);
            writePage(lastPageNum, lastPage);
            std::cout << "Added index entry to page " << lastPageNum
                      << " at position " << currentCount << std::endl;
        }
        else
        {

            int newPageNum = lastPageNum + 1;
            std::vector<std::byte> newPage(PAGE_SIZE, static_cast<std::byte>(0));
            setEntryCount(newPage, 0);
            writeEntry(newPage, 0, studentId, pageId, slot);
            setEntryCount(newPage, 1);
            writePage(newPageNum, newPage);
            std::cout << "Created new index page " << newPageNum
                      << " with first entry (page " << lastPageNum << " was full)" << std::endl;
        }
    }

    bool findIndex(int studentId, int &outPageId, int &outSlot)
    {
        int pageNum = 0;

        while (true)
        {
            std::vector<std::byte> page = readPage(pageNum);

            // Check if page is empty (all zeros)
            bool isEmpty = true;
            for (int i = 0; i < 4; i++)
            {
                if (page[i] != static_cast<std::byte>(0))
                {
                    isEmpty = false;
                    break;
                }
            }
            if (isEmpty && pageNum > 0)
                break;

            int entryCount = getEntryCount(page);
            if (entryCount == 0 && pageNum > 0)
                break;

            // Search through entries in this page
            for (int i = 0; i < entryCount; i++)
            {
                int offset = 4 + (i * ENTRY_SIZE);

                // Extract student_id
                int sid = 0;
                for (int j = 0; j < 4; j++)
                {
                    sid |= (static_cast<int>(page[offset + j]) << (j * 8));
                }

                if (sid == studentId)
                {
                    // Extract page_id
                    int pid = 0;
                    for (int j = 0; j < 4; j++)
                    {
                        pid |= (static_cast<int>(page[offset + 4 + j]) << (j * 8));
                    }

                    // Extract slot
                    int slt = 0;
                    for (int j = 0; j < 4; j++)
                    {
                        slt |= (static_cast<int>(page[offset + 8 + j]) << (j * 8));
                    }

                    outPageId = pid;
                    outSlot = slt;
                    return true;
                }
            }

            pageNum++;
        }

        return false;
    }

    int getLastPageNumber()
    {
        std::fstream file(filename, std::ios::binary | std::ios::in);
        if (!file.is_open())
        {
            return -1;
        }

        file.seekg(0, std::ios::end);
        std::streamsize fileSize = file.tellg();
        file.close();

        if (fileSize == 0)
            return -1;

        return static_cast<int>((fileSize - 1) / PAGE_SIZE);
    }

    void displayAllEntries()
    {
        int pageNum = 0;
        std::cout << "\n===== INDEX CONTENTS =====" << std::endl;

        while (true)
        {
            std::vector<std::byte> page = readPage(pageNum);

            // Check if page exists
            std::fstream testFile(filename, std::ios::binary | std::ios::in);
            if (!testFile.is_open())
                break;
            testFile.seekg(static_cast<std::streamoff>(pageNum) * PAGE_SIZE, std::ios::beg);
            char testByte;
            testFile.get(testByte);
            if (testFile.eof())
                break;
            testFile.close();

            int entryCount = getEntryCount(page);
            if (entryCount == 0 && pageNum > 0)
                break;

            std::cout << "Page " << pageNum << " (count=" << entryCount << "):" << std::endl;
            for (int i = 0; i < entryCount; i++)
            {
                int offset = 4 + (i * ENTRY_SIZE);

                int sid = 0;
                for (int j = 0; j < 4; j++)
                {
                    sid |= (static_cast<int>(page[offset + j]) << (j * 8));
                }

                int pid = 0;
                for (int j = 0; j < 4; j++)
                {
                    pid |= (static_cast<int>(page[offset + 4 + j]) << (j * 8));
                }

                int slt = 0;
                for (int j = 0; j < 4; j++)
                {
                    slt |= (static_cast<int>(page[offset + 8 + j]) << (j * 8));
                }

                std::cout << "  [" << i << "] student_id=" << sid
                          << " → (page=" << pid << ", slot=" << slt << ")" << std::endl;
            }
            pageNum++;
        }
        std::cout << "==========================\n"
                  << std::endl;
    }

private:
    std::string filename = "index.db";
    static constexpr int PAGE_SIZE = 512;
    static constexpr int ENTRIES_PER_PAGE = 42;
    static constexpr int ENTRY_SIZE = 12;

    std::vector<std::byte> readPage(int pageNum)
    {
        std::fstream file(filename, std::ios::binary | std::ios::in);
        std::streamoff offset = static_cast<std::streamoff>(pageNum) * PAGE_SIZE;

        std::vector<std::byte> page(PAGE_SIZE, static_cast<std::byte>(0));

        if (file.is_open())
        {
            file.seekg(offset, std::ios::beg);
            file.read(reinterpret_cast<char *>(page.data()), PAGE_SIZE);
            file.close();
        }
        return page;
    }

    void writePage(int pageNum, std::vector<std::byte> &page)
    {
        std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
        if (!file.is_open())
        {
            // Create file if doesn't exist
            file.open(filename, std::ios::binary | std::ios::out);
            file.close();
            file.open(filename, std::ios::binary | std::ios::in | std::ios::out);
        }

        std::streamoff offset = static_cast<std::streamoff>(pageNum) * PAGE_SIZE;
        file.seekp(offset, std::ios::beg);
        file.write(reinterpret_cast<const char *>(page.data()), PAGE_SIZE);
        file.close();
    }

    int getEntryCount(std::vector<std::byte> &page)
    {
        int count = 0;
        for (int i = 0; i < 4; i++)
        {
            count |= (static_cast<int>(page[i]) << (i * 8));
        }
        return count;
    }

    void setEntryCount(std::vector<std::byte> &page, int count)
    {
        for (int i = 0; i < 4; i++)
        {
            uint8_t byte = (count >> (i * 8)) & 0xFF;
            page[i] = static_cast<std::byte>(byte);
        }
    }

    void writeEntry(std::vector<std::byte> &page, int index, int studentId, int pageId, int slot)
    {
        int offset = 4 + (index * ENTRY_SIZE);

        for (int i = 0; i < 4; i++)
        {
            uint8_t byte = (studentId >> (i * 8)) & 0xFF;
            page[offset + i] = static_cast<std::byte>(byte);
        }

        for (int i = 0; i < 4; i++)
        {
            uint8_t byte = (pageId >> (i * 8)) & 0xFF;
            page[offset + 4 + i] = static_cast<std::byte>(byte);
        }

        for (int i = 0; i < 4; i++)
        {
            uint8_t byte = (slot >> (i * 8)) & 0xFF;
            page[offset + 8 + i] = static_cast<std::byte>(byte);
        }
    }
};

#endif