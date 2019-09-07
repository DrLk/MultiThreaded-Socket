#pragma once

#include <list>

#include "IPacket.h"
#include "SelectPipe.h"

class SelectWriteThread
{
public:
    SelectWriteThread(SelectPipe& pipe);
    SelectPipe& GetPipe();

    void WriteSocket();

private:
    SelectPipe& pipe;
    std::list<IPacket*> writeQueueThread;
};
