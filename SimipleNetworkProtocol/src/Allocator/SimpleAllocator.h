#pragma once
#include<vector>
#include<memory>
#include"SimplePtr.h"

template<class T>
class SimpleAllocator
{
private:
    std::vector<SimplePtr<T>> _usedPtrs;
    std::vector<SimplePtr<T>> _freePtrs;

public:
    SimpleAllocator(size_t size) : _usedPtrs(size, 0), _freePtrs(size)
    {
        for (int i = 0; i < size; i++)
        {
            _freePtrs = SimplePtr<T>(std::make_shared<T>());
        }
    }

    SimplePtr<T> New(int size)
    {
        return _freePtrs[0];
    }
};