#ifndef RECORDSEARCHER
#define RECORDSEARCHER

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "Record.hpp"
#include "recordLayout.hpp"
#include "pageLayout.hpp"
#include "BufferPool.hpp"
#include "IndexManager.hpp"

class RecordSearcher
{
private:
    std::string filename;
    RecordLayout serializer;
    PageLayout pageManager;
    BufferPool &bufferPool;
    IndexManager &indexManager;

    // Counter for I/O operations
    int ioCount;

    // Helper to reset I/O counter
    void resetIOCount()
    {
        ioCount = 0;
    }

    // Helper to increment I/O count (called whenever disk read happens)
    void incrementIO()
    {
        ioCount++;
    }

    // Get total pages in the database file
    int getTotalPages()
    {
        std::fstream file(filename, std::ios::binary | std::ios::in);
        if (!file.is_open())
        {
            return 0;
        }

        file.seekg(0, std::ios::end);
        std::streamsize fileSize = file.tellg();
        file.close();

        return static_cast<int>(fileSize / 512);
    }

public:
    RecordSearcher(std::string &fname, BufferPool &bp, IndexManager &im)
        : filename(fname), bufferPool(bp), indexManager(im)
    {
        ioCount = 0;
    }

    // Method 1: Search using Index (2 I/O operations: index page + data page)
    struct SearchResult searchWithIndex(int studentId)
    {
        resetIOCount();
        SearchResult result;
        result.studentId = studentId;
        result.method = "With Index";

        int pageId = -1;
        int slot = -1;

        // First, find in index (this will read index pages)
        // Note: indexManager.findIndex internally reads index pages
        // We need to count those I/O operations
        bool found = findIndexWithIOcounting(studentId, pageId, slot);

        if (!found)
        {
            result.found = false;
            result.ioCount = ioCount;
            result.message = "Record not found in index";
            return result;
        }

        // Now read the data page (this is one I/O operation if not in buffer)
        // getPage might hit buffer, but we count it as potential I/O
        try
        {
            // Force a fresh read to count I/O accurately
            std::vector<std::byte> pageBytes = readPageDirect(filename, pageId);
            incrementIO(); // Count the data page read

            std::vector<std::byte> recordBytes = pageManager.getRecordFromPage(pageBytes, slot);
            result.record = serializer.deserialize(recordBytes);
            result.found = true;
            result.ioCount = ioCount;
            result.pageId = pageId;
            result.slot = slot;
            result.message = "Found using index";
        }
        catch (const std::exception &e)
        {
            result.found = false;
            result.ioCount = ioCount;
            result.message = "Error reading data page: " + std::string(e.what());
        }

        return result;
    }

    // Method 2: Search without Index (scan all data pages)
    struct SearchResult searchWithoutIndex(int studentId)
    {
        resetIOCount();
        SearchResult result;
        result.studentId = studentId;
        result.method = "Without Index (Full Scan)";

        int totalPages = getTotalPages();

        for (int pageNum = 0; pageNum < totalPages; pageNum++)
        {
            // Read each data page (count I/O)
            std::vector<std::byte> pageBytes = readPageDirect(filename, pageNum);
            incrementIO();

            // Get record count from page header
            int recordCount = 0;
            for (int i = 0; i < 4; i++)
            {
                recordCount |= (static_cast<int>(pageBytes[i]) << (i * 8));
            }

            // Scan all records in this page
            for (int slot = 1; slot <= recordCount; slot++)
            {
                try
                {
                    std::vector<std::byte> recordBytes = pageManager.getRecordFromPage(pageBytes, slot);
                    record r = serializer.deserialize(recordBytes);

                    if (r.id == studentId)
                    {
                        result.record = r;
                        result.found = true;
                        result.ioCount = ioCount;
                        result.pageId = pageNum;
                        result.slot = slot;
                        result.message = "Found after scanning " + std::to_string(pageNum + 1) + " pages";
                        return result;
                    }
                }
                catch (const std::exception &e)
                {
                    // Skip corrupted records
                    continue;
                }
            }
        }

        result.found = false;
        result.ioCount = ioCount;
        result.message = "Record not found after scanning " + std::to_string(totalPages) + " pages";
        return result;
    }

    // Helper: Read page directly to count I/O (bypassing buffer for accurate counting)
    std::vector<std::byte> readPageDirect(std::string &fname, int pageNum)
    {
        std::fstream file(fname, std::ios::binary | std::ios::in);
        std::streamoff offset = static_cast<std::streamoff>(pageNum) * 512;

        std::vector<std::byte> pageBytes(512, static_cast<std::byte>(0));

        if (file.is_open())
        {
            file.seekg(offset, std::ios::beg);
            file.read(reinterpret_cast<char *>(pageBytes.data()), 512);
            file.close();
        }

        return pageBytes;
    }

    // Helper to find in index with I/O counting
    bool findIndexWithIOcounting(int studentId, int &outPageId, int &outSlot)
    {
        int pageNum = 0;

        while (true)
        {
            std::vector<std::byte> page = readPageDirectIndex(pageNum);
            incrementIO(); // Count index page read

            // Check if page is empty
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

            int entryCount = 0;
            for (int i = 0; i < 4; i++)
            {
                entryCount |= (static_cast<int>(page[i]) << (i * 8));
            }
            if (entryCount == 0 && pageNum > 0)
                break;

            // Search through entries in this page
            for (int i = 0; i < entryCount; i++)
            {
                int offset = 4 + (i * 12);

                int sid = 0;
                for (int j = 0; j < 4; j++)
                {
                    sid |= (static_cast<int>(page[offset + j]) << (j * 8));
                }

                if (sid == studentId)
                {
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

                    outPageId = pid;
                    outSlot = slt;
                    return true;
                }
            }

            pageNum++;
        }

        return false;
    }

    std::vector<std::byte> readPageDirectIndex(int pageNum)
    {
        std::string indexFilename = "index.db";
        std::fstream file(indexFilename, std::ios::binary | std::ios::in);
        std::streamoff offset = static_cast<std::streamoff>(pageNum) * 512;

        std::vector<std::byte> pageBytes(512, static_cast<std::byte>(0));

        if (file.is_open())
        {
            file.seekg(offset, std::ios::beg);
            file.read(reinterpret_cast<char *>(pageBytes.data()), 512);
            file.close();
        }

        return pageBytes;
    }
};

// SearchResult structure to hold search results
struct SearchResult
{
    int studentId;
    std::string method;
    bool found;
    record record;
    int ioCount;
    int pageId;
    int slot;
    std::string message;

    void print() const
    {
        std::cout << "\n========================================" << std::endl;
        std::cout << "Search Method: " << method << std::endl;
        std::cout << "Student ID: " << studentId << std::endl;
        std::cout << "----------------------------------------" << std::endl;

        if (found)
        {
            std::cout << "Status: FOUND" << std::endl;
            std::cout << "Location: page " << pageId << ", slot " << slot << std::endl;
            std::cout << "Record: id=" << record.id
                      << ", name='" << record.name
                      << "', grade=" << record.grade
                      << ", is_active=" << (record.isActive ? "true" : "false") << std::endl;
        }
        else
        {
            std::cout << "Status: NOT FOUND" << std::endl;
        }

        std::cout << "I/O Operations: " << ioCount << " page read(s)" << std::endl;
        std::cout << "Message: " << message << std::endl;
        std::cout << "========================================\n"
                  << std::endl;
    }
};

#endif