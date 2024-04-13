#include "LockedList.hpp"
#include <gtest/gtest.h>

#include <stop_token>
#include <utility>

#include "MultiList.hpp"

using FastTransport::Containers::LockedList;
using FastTransport::Containers::MultiList;

TEST(LockedListTest, Wait)
{
    LockedList<int> list;
    const std::stop_token token;

    list.LockedPushBack(1);

    EXPECT_TRUE(list.Wait(token));
}

TEST(LockedListTest, WaitWithPredicate)
{
    LockedList<int> list;
    const std::stop_token token;

    list.LockedPushBack(1);

    const bool result = list.Wait(token, [] { return false; });

    EXPECT_TRUE(result);
}

TEST(LockedListTest, WaitForTrue)
{
    LockedList<int> list;
    const std::stop_token token;

    list.LockedPushBack(1);

    EXPECT_TRUE(list.WaitFor(token));
}

TEST(LockedListTest, WaitForFalse)
{
    LockedList<int> list;
    const std::stop_token token;

    EXPECT_FALSE(list.WaitFor(token));
}

TEST(LockedListTest, WaitForWithPredicate)
{
    LockedList<int> list;
    const std::stop_token token;

    list.LockedPushBack(1);

    const bool result = list.WaitFor(token, [] { return false; });

    EXPECT_FALSE(result);
}

TEST(LockedListTest, NotifyAll)
{
    LockedList<int> list;
    const std::stop_token token;

    list.NotifyAll();

    EXPECT_TRUE(list.LockedTryGenerate(4).empty());
}

TEST(LockedListTest, LockedSplice)
{
    LockedList<int> list1;
    MultiList<int> list2;

    list1.LockedPushBack(1);
    list1.LockedPushBack(2);

    list2.push_back(3);
    list2.push_back(4);

    list1.LockedSplice(std::move(list2));

    MultiList<int> result1 = list1.LockedTryGenerate(4);

    EXPECT_EQ(result1.size(), 4);
    EXPECT_EQ(result1.front(), 1);
    EXPECT_EQ(result1.back(), 4);
}

TEST(LockedListTest, LockedSwap)
{
    LockedList<int> list1;
    MultiList<int> list2;

    list1.LockedPushBack(1);
    list1.LockedPushBack(2);

    list2.push_back(3);
    list2.push_back(4);

    list1.LockedSwap(list2);

    MultiList<int> result1 = list1.LockedTryGenerate(4);

    EXPECT_EQ(result1.size(), 2);
    EXPECT_EQ(result1.front(), 3);
    EXPECT_EQ(result1.back(), 4);

    EXPECT_EQ(list2.size(), 2);
    EXPECT_EQ(list2.front(), 1);
    EXPECT_EQ(list2.back(), 2);
}

TEST(LockedListTest, LockedTryGenerate)
{
    LockedList<int> list;

    list.LockedPushBack(1);
    list.LockedPushBack(2);

    auto result = list.LockedTryGenerate(2);

    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result.front(), 1);
    EXPECT_EQ(result.back(), 2);
}

TEST(LockedListTest, LockedPushBack)
{
    LockedList<int> list;

    list.LockedPushBack(1);

    MultiList<int> result = list.LockedTryGenerate(2);

    EXPECT_EQ(result.front(), 1);
    EXPECT_EQ(result.back(), 1);
}

TEST(LockedListTest, LockedGetBack)
{
    LockedList<int> list;

    list.LockedPushBack(1);

    const int result = list.LockedGetBack();

    const MultiList<int> resultList = list.LockedTryGenerate(2);

    EXPECT_EQ(result, 1);
    EXPECT_TRUE(resultList.empty());
}

TEST(MultiListTest, Splice)
{
    MultiList<int> list;
    for (int i = 0; i < 100; i++) {
        list.push_back(std::move(i));
    }

    MultiList<int> freePackets;
    auto _packet = list.begin();
    _packet++;
    if (_packet != list.begin()) {
        auto begin = _packet;
        begin++;
        freePackets.splice(list, begin, list.end());
    }

    MultiList<int> packets;
    for (int i = 0; i < 64; i++) {
        packets.push_back(std::move(i));
    }

    _packet = packets.end();
    _packet--;
    list.splice(std::move(packets));
    list.splice(std::move(freePackets));

    _packet++;
    assert(_packet != list.end());
}

TEST(MultiListTest, Splice2)
{
    int size = 64;
    MultiList<int> list;
    for (int i = 0; i < 100; i++) {
        list.push_back(std::move(i));
    }

    MultiList<int> freePackets;
    auto begin = list.begin();
    freePackets.splice(list, begin, list.end());
    EXPECT_EQ(list.size(), 0);
    EXPECT_EQ(freePackets.size(), 100);

    MultiList<int> result = freePackets.TryGenerate(size);

    list.splice(std::move(freePackets));
}
