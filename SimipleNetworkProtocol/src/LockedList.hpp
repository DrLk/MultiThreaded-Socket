#pragma once

#include <condition_variable>
#include <mutex>

#include "MultiList.hpp"

namespace FastTransport::Containers {

template <class T>
class LockedList : public FastTransport::Containers::MultiList<T> {
public:
    LockedList& operator=(FastTransport::Containers::MultiList<T>&& that); // NOLINT(fuchsia-overloaded-operator)

    template <class Predicate>
    void Wait(std::unique_lock<std::mutex>& lock, Predicate&& predicate);
    void NotifyAll() noexcept;

    std::mutex _mutex; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)
private:
    std::condition_variable _condition {};
};

template <class T> // NOLINT(fuchsia-overloaded-operator)
LockedList<T>& LockedList<T>::operator=(FastTransport::Containers::MultiList<T>&& that)
{
    FastTransport::Containers::MultiList<T>::operator=(std::move(that));
    return *this;
}

template <class T>
template <class Predicate>
void LockedList<T>::Wait(std::unique_lock<std::mutex>& lock, Predicate&& predicate)
{
    _condition.wait(lock, std::forward<Predicate>(predicate));
}

template <class T>
void LockedList<T>::NotifyAll() noexcept
{
    _condition.notify_all();
}
} // namespace FastTransport::Containers
