#pragma once

#include <mutex>
#include <list>

template<class T>
class LockedList : public std::list<T>
{
public:
    std::mutex _mutex;
};
