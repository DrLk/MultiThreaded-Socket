#pragma once

#include <mutex>

#include "MultiList.h"

template<class T>
class LockedList : public FastTransport::Containers::MultiList<T>
{
public:

    LockedList& operator=(FastTransport::Containers::MultiList<T>&& that)
    {
        FastTransport::Containers::MultiList::operator=(std::move(that));
        return *this;
    }

    std::mutex _mutex;
};
