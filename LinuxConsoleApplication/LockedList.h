#pragma once

#include <mutex>
#include <list>

template<class T>
class LockedList
{
public:
    std::mutex _mutex;
    std::list<T> _list;

    auto begin()
    {
        return _list.begin();
    }

    auto end()
    {
        return _list.end();
    }

    void push_back(const T& value)
    {
        _list.push_back(value);
    }

    void push_back(T&& value)
    {
        _list.push_back(value);
    }

    void resize(size_t size)
    {
        _list.resize(size);
    }

    auto size()
    {
        return _list.size();
    }

    void pop_back()
    {
        _list.pop_back();
    }

    auto back()
    {
        return _list.back();
    }

    auto empty()
    {
        return _list.empty();
    }

    void splice(typename std::list<T>::const_iterator position, std::list<T>&& that)
    {
        _list.splice(position, that);
    }

    void splice(typename std::list<T>::const_iterator position, const std::list<T>& that)
    {
        _list.splice(position, that);
    }

    void splice(typename std::list<T>::const_iterator position, const std::list<T>& that, typename std::list<T>::const_iterator it)
    {
        _list.splice(position, that, it);
    }

    void splice(typename std::list<T>::const_iterator position, const std::list<T>& that, typename std::list<T>::const_iterator first, typename std::list<T>::const_iterator last)
    {
        _list.splice(position, that, first, last);
    }
};
