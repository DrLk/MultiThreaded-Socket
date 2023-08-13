#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <stop_token>
#include <utility>

#include "MultiList.hpp"
#include "SpinLock.hpp"

namespace FastTransport::Containers {

using namespace std::chrono_literals;

template <class T>
class LockedList final : private FastTransport::Containers::MultiList<T> {
public:
    LockedList();
    LockedList(const LockedList& that) = delete;
    LockedList(LockedList&& that) noexcept;
    LockedList& operator=(const LockedList& that) = delete;
    LockedList& operator=(LockedList&& that) noexcept; // NOLINT(fuchsia-overloaded-operator)
    ~LockedList() override;

    bool Wait(std::stop_token stop);
    template <class Predicate>
    bool Wait(std::stop_token stop, Predicate&& predicate);
    bool WaitFor(std::stop_token stop);
    template <class Predicate>
    bool WaitFor(std::stop_token stop, Predicate&& predicate);
    void NotifyAll() noexcept;
    void LockedSplice(LockedList&& list) = delete;
    void LockedSplice(MultiList<T>&& list);
    void LockedSwap(MultiList<T>& list);
    MultiList<T> LockedTryGenerate(size_t size);
    void LockedPushBack(T&& element);
    T LockedGetBack();

private:
    using Mutex = FastTransport::Protocol::SpinLock;

    Mutex _mutex;
    std::condition_variable_any _condition {};
};

template <class T>
LockedList<T>::LockedList() = default;

template <class T>
LockedList<T>::LockedList(LockedList&& that) noexcept
    : FastTransport::Containers::MultiList<T>(std::move(that))
{
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
LockedList<T>& LockedList<T>::operator=(LockedList&& that) noexcept
{
    FastTransport::Containers::MultiList<T>::operator=(std::move(that));
    return *this;
}

template <class T>
LockedList<T>::~LockedList() = default;

template <class T>
bool LockedList<T>::Wait(std::stop_token stop)
{
    std::unique_lock<Mutex> lock(_mutex);
    return _condition.wait(lock, stop, [this]() { return !MultiList<T>::empty(); });
}

template <class T>
template <class Predicate>
bool LockedList<T>::Wait(std::stop_token stop, Predicate&& predicate)
{
    auto fullPredicate = [this, &predicate]() { return !MultiList<T>::empty() || predicate(); };
    std::unique_lock<Mutex> lock(_mutex);
    return _condition.wait(lock, stop, std::move(fullPredicate));
}

template <class T>
bool LockedList<T>::WaitFor(std::stop_token stop)
{
    std::unique_lock<Mutex> lock(_mutex);
    return _condition.wait_for(lock, stop, 50ms, [this]() { return !MultiList<T>::empty(); });
}

template <class T>
template <class Predicate>
bool LockedList<T>::WaitFor(std::stop_token stop, Predicate&& predicate)
{
    std::unique_lock<Mutex> lock(_mutex);
    return _condition.wait_for(lock, stop, 50ms, std::forward<Predicate>(predicate));
}

template <class T>
void LockedList<T>::NotifyAll() noexcept
{
    _condition.notify_all();
}

template <class T>
void LockedList<T>::LockedSplice(MultiList<T>&& list)
{
    const std::scoped_lock<Mutex> lock(_mutex);

    MultiList<T>::splice(std::move(list));
}

template <class T>
void LockedList<T>::LockedSwap(MultiList<T>& list)
{
    const std::scoped_lock<Mutex> lock(_mutex);

    MultiList<T>::swap(list);
}

template <class T>
MultiList<T> LockedList<T>::LockedTryGenerate(size_t size)
{
    MultiList<T> result;
    const std::scoped_lock<Mutex> lock(_mutex);

    if (size < MultiList<T>::size())
        result = MultiList<T>::TryGenerate(size);
    else
        MultiList<T>::swap(result);

    return result;
}

template <class T>
void LockedList<T>::LockedPushBack(T&& element)
{
    const std::scoped_lock<Mutex> lock(_mutex);

    MultiList<T>::push_back(std::move(element));
}

template <class T>
T LockedList<T>::LockedGetBack()
{
    const std::scoped_lock<Mutex> lock(_mutex);

    T result = std::move(MultiList<T>::back());
    MultiList<T>::pop_back();
    return result;
}
} // namespace FastTransport::Containers
