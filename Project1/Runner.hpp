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
#include "RecordSearcher.hpp"


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
        
        int recordCountInCurrentPage = 0;

        // 2. Insert 20 student records
        std::cout << "=== Inserting 20 Student Records ===" << std::endl;
        for (int i = 0; i < 20; i++)
        {
            int id = i;
            std::string name = "Student" + std::to_string(i);
            float grade = 10.0f + (i % 15);
            bool is_active = (i % 2 == 0);

            record r;
            r.id = id;
            r.name = name;
            r.grade = grade;
            r.isActive = is_active;

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
                    
                    int currentCount = 0;
                    for (int j = 0; j < 4; j++)
                    {
                        currentCount |= (static_cast<int>(currentPage[j]) << (j * 8));
                    }
                    finalSlotNum = currentCount;
                    
                    finalPageNum = current_page_num;
                    added = true;
                }
                catch (const std::overflow_error &e)
                {
                    bufferPool.markDirty(current_page_num);
                    current_page_num++;
                    recordCountInCurrentPage = 0;
                    currentPage = bufferPool.getPage(filename, current_page_num);
                }
            }

            indexManager.insertIndex(id, finalPageNum, finalSlotNum);
            
            std::cout << "Inserted record " << i << ": ID=" << id
                      << ", Name=" << name << ", Grade=" << grade
                      << ", Active=" << is_active
                      << " → Index: (page=" << finalPageNum << ", slot=" << finalSlotNum << ")" << std::endl;
        }

        bufferPool.markDirty(current_page_num);
        bufferPool.flushAll(filename);
        
        // Display index contents
        indexManager.displayAllEntries();
        
        // Run search comparison
        runSearchComparison();
    }

    // Function to test and compare both search methods
    void runSearchComparison()
    {
        std::string filename = "students.db";
        
        std::fstream file(filename, std::ios::binary | std::ios::in);
        if (!file.is_open()) {
            std::cout << "Database file not found. Please insert records first." << std::endl;
            return;
        }
        file.close();
        
        RecordLayout serializer;
        PageLayout pageManager;
        BufferPool bufferPool(3);
        IndexManager indexManager;
        
        RecordSearcher searcher(filename, bufferPool, indexManager);
        
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║           SEARCH COMPARISON: WITH INDEX vs WITHOUT INDEX     ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
        
        // Test 1: Search for existing record (id=5)
        std::cout << "\n🔍 TEST 1: Searching for existing record (ID=5)" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        auto resultWithIndex = searcher.searchWithIndex(5);
        resultWithIndex.print();
        
        auto resultWithoutIndex = searcher.searchWithoutIndex(5);
        resultWithoutIndex.print();
        
        // Test 2: Search for another existing record (id=12)
        std::cout << "\n🔍 TEST 2: Searching for existing record (ID=12)" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        resultWithIndex = searcher.searchWithIndex(12);
        resultWithIndex.print();
        
        resultWithoutIndex = searcher.searchWithoutIndex(12);
        resultWithoutIndex.print();
        
        // Test 3: Search for non-existing record (id=999)
        std::cout << "\n🔍 TEST 3: Searching for non-existing record (ID=999)" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        resultWithIndex = searcher.searchWithIndex(999);
        resultWithIndex.print();
        
        resultWithoutIndex = searcher.searchWithoutIndex(999);
        resultWithoutIndex.print();

    }

private:

};

#endif