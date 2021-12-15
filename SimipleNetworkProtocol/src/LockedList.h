#pragma once

#include <mutex>

#include "MultiList.h"

using namespace FastTransport::Containers;

template<class T>
class LockedList : public MultiList<T>
{
public:

    LockedList& operator=(MultiList<T>&& that)
    {
        MultiList<T>::operator=(std::move(that));
        return *this;
    }

    std::mutex _mutex;
};
