#ifndef RUNNER
#define RUNNER
#include <iostream>
#include <string>
#include <vector>
#include <cstddef>
#include <fstream>
#include "Record.hpp"
#include "recordLayout.hpp"
#include "pageLayout.hpp"
#include "BufferPool.hpp"


class Runner
{
public:
    void run()
    {
        std::string filename = "students.db";

        // 1. Open the file "students.db" (mode 'rb+', create if it doesn't exist)
        std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);

        if (!file.is_open())
        {
            std::ofstream createFile(filename, std::ios::binary);
            createFile.close();
            file.open(filename, std::ios::binary | std::ios::in | std::ios::out);
        }
        file.close();

        RecordLayout serializer;
        PageLayout pageManager;
        BufferPool bufferPool(3);

        int current_page_num = 0;
        std::vector<std::byte> currentPage = bufferPool.getPage(filename, current_page_num);

        // 2. Insert 20 student records
        std::cout << "=== Inserting 20 Student Records ===" << std::endl;
        for (int i = 0; i < 20; i++)
        {
            int id = i;
            std::string name = "Student" + std::to_string(i);
            float grade = 10.0f + (i % 15);
            bool is_active = (i % 2 == 0); // Even IDs are active

            // Create record
            record r;
            r.id = id;
            r.name = name;
            r.grade = grade;
            r.isActive = is_active;

            // Serialize record to bytes
            std::vector<std::byte> recordBytes = serializer.serializeRecord(r);

            bool added = false;
            while (!added)
            {
                try
                {
                    currentPage = pageManager.addRecordToPage(currentPage, recordBytes);
                    bufferPool.markDirty(current_page_num);
                    added = true;
                }
                catch (const std::overflow_error &e)
                {
                    // Page is full, move to next page
                    bufferPool.markDirty(current_page_num);
                    current_page_num++;
                    currentPage = bufferPool.getPage(filename, current_page_num);
                    // Try again with new page
                }
            }

            std::cout << "Inserted record " << i << ": ID=" << id
                      << ", Name=" << name << ", Grade=" << grade
                      << ", Active=" << is_active << std::endl;
        }

        // Mark the last page as dirty
        bufferPool.markDirty(current_page_num);

        // 3. Find and display record #5 (id=5)
        std::cout << "\n=== Finding Record #5 (id=5) ===" << std::endl;
        int targetId = 5;

        // Find which page contains this record (page = 5 // 16 = 0)
        int pageNum = targetId / 16;          // 16 records per page (512 bytes - 4 header = 508 / 30 ≈ 16)
        int slotNumber = (targetId % 16) + 1; // Slot numbers start at 1

        std::cout << "Record #5 is on page " << pageNum << ", slot " << slotNumber << std::endl;

        try
        {
            // Get the page from buffer
            std::vector<std::byte> pageBytes = bufferPool.getPage(filename, pageNum);

            // Get the record from the page
            std::vector<std::byte> recordBytes = pageManager.getRecordFromPage(pageBytes, slotNumber);

            // Deserialize the record
            record foundRecord = serializer.deserialize(recordBytes);

            // Print the record
            std::cout << "Record #5: id=" << foundRecord.id
                      << ", name='" << foundRecord.name
                      << "', grade=" << foundRecord.grade
                      << ", is_active=" << (foundRecord.isActive ? "true" : "false") << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error finding record #5: " << e.what() << std::endl;
        }

        // 4. Write all dirty pages to disk (flush_all)
        std::cout << "\n=== Flushing All Dirty Pages to Disk ===" << std::endl;
        bufferPool.flushAll(filename);

        // 5. Close the file
        std::cout << "\n=== Closing File ===" << std::endl;
        // File is already closed by flushAll operations, but we can ensure it
        std::fstream closeFile(filename, std::ios::binary | std::ios::in | std::ios::out);
        closeFile.close();

        // 6. Print "Done."
        std::cout << "\n=== Done. ===" << std::endl;
    }

private:

};


#endif