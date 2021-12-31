#pragma once

#include <mutex>

#include "MultiList.hpp"

template <class T>
class LockedList : public FastTransport::Containers::MultiList<T> {
public:
    LockedList& operator=(FastTransport::Containers::MultiList<T>&& that) // NOLINT(fuchsia-overloaded-operator)
    {
        FastTransport::Containers::MultiList<T>::operator=(std::move(that));
        return *this;
    }

    std::mutex _mutex; // NOLINT(misc-non-private-member-variables-in-classes)
};
