#ifndef BUFFERPOOL
#define BUFFERPOOL
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstddef>
#include <list>
#include <string>
#include <algorithm>
#include<iostream>
#include"FileManager.hpp"

class BufferPool
{

public:
    BufferPool(int capacity = 3) : capacity(capacity) {}

    std::vector<std::byte> getPage(std::string &fileName, int pageNum)
    {
        // if pageNum is in pages cache
        if (pages.find(pageNum) != pages.end())
        {
            // move it to the end of the lru
            auto it = std::find(lru.begin(), lru.end(), pageNum);
            if (it != lru.end())
            {
                lru.erase(it);
            }
            lru.push_back(pageNum);
            return pages[pageNum];
        }
        // If not in cache, read from disk
        std::vector<std::byte> pageBytes = fm.readPage(fileName, pageNum);

        // If cache is full, evict oldest page
        if (pages.size() >= static_cast<size_t>(capacity))
        {
            int oldest = lru.front();
            lru.pop_front();

            // Check if oldest page is dirty
            if (dirty.find(oldest) != dirty.end())
            {
                fm.writePage(fileName, oldest, pages[oldest]);
                std::cout << "Flushed page " << oldest << " to disk" << std::endl;
            }

            // Remove from cache
            pages.erase(oldest);
            dirty.erase(oldest);
            std::cout << "Buffer evicted: page " << oldest << std::endl;
        }

        // Add new page to cache
        pages[pageNum] = pageBytes;
        lru.push_back(pageNum);

        return pageBytes;
    }

    void markDirty(int pageNum){
        dirty.emplace(pageNum);
    }


    void flushAll (std::string fileName){
        
    }

private:
    int capacity;
    std::unordered_map<int, std::vector<std::byte>> pages;
    std::list<int> lru;
    std::unordered_set<int> dirty;
    FileManager fm ;
};

#endif