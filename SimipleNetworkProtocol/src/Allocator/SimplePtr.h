#pragma once

#include<memory>

template<class T>
class SimplePtr
{
private:
    std::shared_ptr<T> _ptr;

public:
    //SimplePtr(const std::shared_ptr<T>& ptr) = delete;

    SimplePtr(std::shared_ptr<T>&& ptr) : _ptr(std::move(ptr))
    {
    }

};