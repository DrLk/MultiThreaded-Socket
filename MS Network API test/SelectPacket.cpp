#include "SelectPacket.h"


SelectPacket::SelectPacket(size_t size) : buffer(size)
{
}

void SelectPacket::SetBuffer(std::vector<char> buffer)
{
    this->buffer.assign(buffer.begin(), buffer.end());

}

std::vector<char>& SelectPacket::GetBuffer()
{
    return this->buffer;
}


SOCKADDR_IN& SelectPacket::GetDestination()
{
    return dst;
}

