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
#include "IndexManager.hpp"  


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
        IndexManager indexManager;  

        int current_page_num = 0;
        std::vector<std::byte> currentPage = bufferPool.getPage(filename, current_page_num);
        
        // Variables to track slot number for each record
        int recordCountInCurrentPage = 0;

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
            int finalPageNum = current_page_num;
            int finalSlotNum = 0;
            
            while (!added)
            {
                try
                {
                    currentPage = pageManager.addRecordToPage(currentPage, recordBytes);
                    bufferPool.markDirty(current_page_num);
                    
                    // Calculate slot number (1-based)
                    // First get current record count from page header
                    int currentCount = 0;
                    for (int j = 0; j < 4; j++)
                    {
                        currentCount |= (static_cast<int>(currentPage[j]) << (j * 8));
                    }
                    finalSlotNum = currentCount; // After adding, count is the slot number
                    
                    finalPageNum = current_page_num;
                    added = true;
                }
                catch (const std::overflow_error &e)
                {
                    // Page is full, move to next page
                    bufferPool.markDirty(current_page_num);
                    current_page_num++;
                    recordCountInCurrentPage = 0;
                    currentPage = bufferPool.getPage(filename, current_page_num);
                    // Try again with new page
                }
            }

            // Insert into index (student_id, page_id, slot)
            indexManager.insertIndex(id, finalPageNum, finalSlotNum);
            
            std::cout << "Inserted record " << i << ": ID=" << id
                      << ", Name=" << name << ", Grade=" << grade
                      << ", Active=" << is_active
                      << "  Index: (page=" << finalPageNum << ", slot=" << finalSlotNum << ")" << std::endl;
        }

        // Mark the last page as dirty
        bufferPool.markDirty(current_page_num);

        // Display all index entries (for debugging)
        indexManager.displayAllEntries();

        // 3. Find and display record #5 (id=5) using INDEX
        std::cout << "\n=== Finding Record #5 (id=5) using INDEX ===" << std::endl;
        int targetId = 5;

        int pageNum = -1;
        int slotNumber = -1;
        
        if (indexManager.findIndex(targetId, pageNum, slotNumber))
        {
            std::cout << "Index found: Record #" << targetId 
                      << " is on page " << pageNum << ", slot " << slotNumber << std::endl;
            
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
                std::cerr << "Error reading record #5: " << e.what() << std::endl;
            }
        }
        else
        {
            std::cout << "Record #" << targetId << " not found in index!" << std::endl;
            
            // Fallback to old method (manual calculation)
            std::cout << "\nFalling back to manual calculation..." << std::endl;
            int fallbackPageNum = targetId / 16;
            int fallbackSlotNumber = (targetId % 16) + 1;
            
            std::cout << "Manual: page " << fallbackPageNum << ", slot " << fallbackSlotNumber << std::endl;
            
            try
            {
                std::vector<std::byte> pageBytes = bufferPool.getPage(filename, fallbackPageNum);
                std::vector<std::byte> recordBytes = pageManager.getRecordFromPage(pageBytes, fallbackSlotNumber);
                record foundRecord = serializer.deserialize(recordBytes);
                
                std::cout << "Record #5 (manual): id=" << foundRecord.id
                          << ", name='" << foundRecord.name
                          << "', grade=" << foundRecord.grade
                          << ", is_active=" << (foundRecord.isActive ? "true" : "false") << std::endl;
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }

        // 4. Write all dirty pages to disk (flush_all)
        std::cout << "\n=== Flushing All Dirty Pages to Disk ===" << std::endl;
        bufferPool.flushAll(filename);

        // 5. Close the file
        std::cout << "\n=== Closing File ===" << std::endl;
        std::fstream closeFile(filename, std::ios::binary | std::ios::in | std::ios::out);
        closeFile.close();

        // 6. Print "Done."
        std::cout << "\n=== Done. ===" << std::endl;
    }

private:

};

#endif