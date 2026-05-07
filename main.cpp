#include<iostream>
#include"recordLayout.hpp"

int main(){

    record r ;
    r.name = "Mehrdad" ;
    r.id = 123 ;
    r.grade = 16.3 ;
    r.isActive = 1 ;
    recordLayout rl ;
    std::vector<std::byte> data ;
    data = rl.serializeRecord(r) ;
    record r2 = rl.deserialize(data) ;

    return 0 ;
}