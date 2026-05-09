#ifndef BUFFERPOOL
#define BUFFERPOOL
#include<vector>
#include<unordered_map>
#include<unordered_set>
#include<cstddef>
#include<list>
#include<string>

class BufferPool{

public:
    BufferPool(int capacity =3) : capacity(capacity){}



private:
    int capacity ;
    std::unordered_map<int, std::vector<std::byte>> pages ;
    std::list<int> lru ;
    std::unordered_set<int> dirty ;


};

#endif