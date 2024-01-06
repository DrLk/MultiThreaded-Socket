#pragma once

#include <cstddef>
#include <list>
#include <ranges>

namespace FastTransport::Containers {
template <class T>
class MultiList : std::ranges::view_interface<MultiList<T>> {
public:
    class Iterator {
        friend MultiList;

    public:
        using value_type = T;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;

        Iterator() = default;
        explicit Iterator(MultiList<T>* container);
        Iterator(MultiList<T>* container, const typename std::list<std::list<T>>::iterator& it1);

        Iterator& operator++(); // NOLINT(fuchsia-overloaded-operator)
        const Iterator operator++(int); // NOLINT(fuchsia-overloaded-operator)
        bool operator==(const Iterator& other) const; // NOLINT(fuchsia-overloaded-operator)
        bool operator!=(const Iterator& other) const; // NOLINT(fuchsia-overloaded-operator)
        T& operator*() const; // NOLINT(fuchsia-overloaded-operator)
        const T* operator->() const; // NOLINT(fuchsia-overloaded-operator)
        T* operator->(); // NOLINT(fuchsia-overloaded-operator)

    private:
        const MultiList<T>* _container;

        typename std::list<std::list<T>>::iterator _it1;
        typename std::list<T>::iterator _it2;
    };

    class ConstIterator {
        friend MultiList;

    public:
        using value_type = T;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;

        ConstIterator() = default;
        explicit ConstIterator(const MultiList<T>* container);
        ConstIterator(const MultiList<T>* container, const typename std::list<std::list<T>>::iterator& it1);

        ConstIterator& operator++(); // NOLINT(fuchsia-overloaded-operator)
        const ConstIterator operator++(int); // NOLINT(fuchsia-overloaded-operator)
        bool operator==(const ConstIterator& other) const; // NOLINT(fuchsia-overloaded-operator)
        bool operator!=(const ConstIterator& other) const; // NOLINT(fuchsia-overloaded-operator)
        const T& operator*() const; // NOLINT(fuchsia-overloaded-operator)
        const T* operator->() const; // NOLINT(fuchsia-overloaded-operator)
        const T* operator->(); // NOLINT(fuchsia-overloaded-operator)

    private:
        const MultiList<T>* _container;

        typename std::list<std::list<T>>::iterator _it1;
        typename std::list<T>::iterator _it2;
    };

    MultiList() = default;
    virtual ~MultiList() = default;

    MultiList(const MultiList& that) = delete;
    MultiList(MultiList&& that) noexcept;
    MultiList& operator=(const MultiList& that) = delete;
    MultiList& operator=(MultiList&& that) noexcept;

    MultiList TryGenerate(std::size_t size);

    void push_back(T&& element);
    void splice(MultiList<T>&& that); // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    [[nodiscard]] bool empty() const noexcept;
    T& front();
    T& back();
    void pop_back();
    [[nodiscard]] std::size_t size() const noexcept;

    Iterator begin();
    Iterator end();
    ConstIterator begin() const;
    ConstIterator end() const;
    ConstIterator cbegin() const;
    ConstIterator cend() const;
    Iterator erase(const Iterator& that);
    void swap(MultiList& that) noexcept;

private:
    std::list<std::list<T>> _lists;

    static constexpr int Size = 10;
};

template <class T>
MultiList<T>::Iterator::Iterator(MultiList<T>* container)
    : _container(container)
    , _it1(container->_lists.begin())
{
    if (_it1 != container->_lists.end()) {
        _it2 = _it1->begin();
    }
}

template <class T>
MultiList<T>::Iterator::Iterator(MultiList* container, const typename std::list<std::list<T>>::iterator& it1)
    : _container(container)
    , _it1(it1)
    , _it2()
{
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
typename MultiList<T>::Iterator& MultiList<T>::Iterator::operator++() // NOLINT(fuchsia-overloaded-operator)
{
    _it2++;
    if (_it2 == _it1->end()) {
        _it1++;
        if (_it1 == _container->_lists.end()) {
            const typename std::list<T>::iterator defaultIterator {};
            _it2 = defaultIterator;
        } else {
            _it2 = _it1->begin();
        }
    }

    return *this;
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
const MultiList<T>::Iterator MultiList<T>::Iterator::operator++(int) // NOLINT(fuchsia-overloaded-operator, readability-const-return-type)
{
    _it2++;
    if (_it2 == _it1->end()) {
        _it1++;
        if (_it1 == _container->_lists.end()) {
            const typename std::list<T>::iterator defaultIterator {};
            _it2 = defaultIterator;
        } else {
            _it2 = _it1->begin();
        }
    }

    return *this;
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
bool MultiList<T>::Iterator::operator==(const Iterator& other) const // NOLINT(fuchsia-overloaded-operator)
{
    if (_it1 == other._it1) {
        if (_it2 == other._it2) {
            return true;
        }

        if (_it1 == _container->_lists.end()) {
            return true;
        }
    }

    return false;
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
bool MultiList<T>::Iterator::operator!=(const Iterator& other) const // NOLINT(fuchsia-overloaded-operator)
{
    return !(*this == other);
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
T& MultiList<T>::Iterator::operator*() const // NOLINT(fuchsia-overloaded-operator)
{
    return *_it2;
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
const T* MultiList<T>::Iterator::operator->() const // NOLINT(fuchsia-overloaded-operator)
{
    return &(*_it2);
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
T* MultiList<T>::Iterator::operator->() // NOLINT(fuchsia-overloaded-operator)
{
    return &(*_it2);
}

template <class T>
MultiList<T>::ConstIterator::ConstIterator(const MultiList* container)
    : _container(container)
    , _it1(container->_lists.begin())
{
    if (_it1 != container->_lists.end()) {
        _it2 = _it1->begin();
    }
}

template <class T>
MultiList<T>::ConstIterator::ConstIterator(const MultiList* container, const typename std::list<std::list<T>>::iterator& it1)
    : _container(container)
    , _it1(it1)
    , _it2()
{
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
typename MultiList<T>::ConstIterator& MultiList<T>::ConstIterator::operator++() // NOLINT(fuchsia-overloaded-operator)
{
    _it2++;
    if (_it2 == _it1->end()) {
        _it1++;
        if (_it1 == _container->_lists.end()) {
            const typename std::list<T>::iterator defaultIterator {};
            _it2 = defaultIterator;
        } else {
            _it2 = _it1->begin();
        }
    }

    return *this;
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
const typename MultiList<T>::ConstIterator MultiList<T>::ConstIterator::operator++(int) // NOLINT(fuchsia-overloaded-operator, readability-const-return-type)
{
    auto copy = operator++();
    operator++();
    return copy;
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
bool MultiList<T>::ConstIterator::operator==(const ConstIterator& other) const // NOLINT(fuchsia-overloaded-operator)
{
    return _it1 == other._it1 && _it2 == other._it2;
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
bool MultiList<T>::ConstIterator::operator!=(const ConstIterator& other) const // NOLINT(fuchsia-overloaded-operator)
{
    return !(*this == other);
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
const T& MultiList<T>::ConstIterator::operator*() const // NOLINT(fuchsia-overloaded-operator)
{
    return *_it2;
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
const T* MultiList<T>::ConstIterator::operator->() const // NOLINT(fuchsia-overloaded-operator)
{
    return &(*_it2);
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
const T* MultiList<T>::ConstIterator::operator->() // NOLINT(fuchsia-overloaded-operator)
{
    return &(*_it2);
}

template <class T>
MultiList<T>::MultiList(MultiList&& that) noexcept
    : _lists(std::move(that._lists))
{
}

template <class T>
MultiList<T>& MultiList<T>::operator=(MultiList&& that) noexcept
{
    _lists = std::move(that._lists);
    return *this;
}

template <class T>
MultiList<T> MultiList<T>::TryGenerate(std::size_t size)
{
    MultiList list;

    while (!_lists.empty() && size) {
        auto& front = _lists.front();
        auto frontSize = front.size();
        if (frontSize > size) {
            list._lists.emplace_back(0);

            auto iterator = front.begin();
            while (size-- != 0) {
                iterator++;
            }
            list._lists.back().splice(list._lists.back().end(), front, front.begin(), iterator);
            break;
        }

        list._lists.push_back(std::move(front));
        _lists.pop_front();

        size -= frontSize;
    }

    return list;
}

template <class T>
void MultiList<T>::push_back(T&& element)
{
    if (_lists.empty()) {
        _lists.emplace_back(0);
    }

    auto& list = _lists.back();
    if (list.size() < Size) {
        list.push_back(std::move(element));
    } else {
        _lists.emplace_back(0);
        _lists.back().push_back(std::move(element));
    }
}

template <class T>
void MultiList<T>::splice(MultiList<T>&& that) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    if (!_lists.empty() && !that._lists.empty()) {
        auto& list = _lists.back();
        auto& thatList = that._lists.front();
        auto newListSize = list.size() + thatList.size();
        if (newListSize < Size) {
            list.splice(list.end(), std::move(thatList));
            that._lists.pop_front();
        }
    }
    _lists.splice(_lists.end(), std::move(that._lists));
}

template <class T>
[[nodiscard]] bool MultiList<T>::empty() const noexcept
{
    return _lists.empty();
}

template <class T>
T& MultiList<T>::front()
{
    return _lists.front().front();
}

template <class T>
T& MultiList<T>::back()
{
    return _lists.back().back();
}

template <class T>
void MultiList<T>::pop_back()
{
    _lists.back().pop_back();

    if (_lists.back().empty()) {
        _lists.pop_back();
    }
}

template <class T>
[[nodiscard]] std::size_t MultiList<T>::size() const noexcept
{
    std::size_t size = 0;
    for (const auto& it1 : _lists) {
        size += it1.size();
    }

    return size;
}

template <class T>
typename MultiList<T>::Iterator MultiList<T>::begin()
{
    return Iterator(this);
}

template <class T>
typename MultiList<T>::ConstIterator MultiList<T>::begin() const
{
    return ConstIterator(this);
}

template <class T>
typename MultiList<T>::ConstIterator MultiList<T>::cbegin() const
{
    return ConstIterator(this);
}

template <class T>
typename MultiList<T>::Iterator MultiList<T>::end()
{
    return Iterator(this, _lists.end());
}

template <class T>
typename MultiList<T>::ConstIterator MultiList<T>::end() const
{
    return ConstIterator(this, _lists.end());
}

template <class T>
typename MultiList<T>::ConstIterator MultiList<T>::cend() const
{
    return ConstIterator(this, _lists.end());
}

template <class T>
MultiList<T>::Iterator MultiList<T>::erase(const Iterator& that)
{
    Iterator newIterator = that;
    newIterator._it2 = newIterator._it1->erase(newIterator._it2);
    if (newIterator._it2 == newIterator._it1->end()) {
        if (newIterator._it1->empty()) {
            newIterator._it1 = _lists.erase(newIterator._it1);
        } else {
            newIterator._it1++;
        }

        if (newIterator._it1 != _lists.end()) {
            newIterator._it2 = newIterator._it1->begin();
        }
    }

    return newIterator;
}

template <class T>
void MultiList<T>::swap(MultiList& that) noexcept
{
    _lists.swap(that._lists);
}

} // namespace FastTransport::Containers
