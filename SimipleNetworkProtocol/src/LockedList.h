#pragma once

#include <mutex>
#include <list>

template<class T>
class LockedList : public std::list<T>
{
public:

    LockedList& operator=(std::list<T>&& that)
    {
        std::list<T>::operator=(std::move(that));
        return *this;
    }

    std::mutex _mutex;
};
