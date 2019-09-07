#include "SelectWriteThread.h"

#ifdef WIN32
#include <ws2tcpip.h> //TODO: remove
#endif

SelectWriteThread::SelectWriteThread(SelectPipe& pipe) : pipe(pipe)
{

}

SelectPipe& SelectWriteThread::GetPipe()
{
    return pipe;
}

#include <atomic>
std::atomic<long> counter = 0;
std::atomic<long> counterError = 0;
std::atomic<long long> writeCounter = 0;
void SelectWriteThread::WriteSocket()
{
    if (writeQueueThread.empty())
    {
        {
            std::lock_guard<std::mutex> lock(GetPipe().writeQueueMutex);
            auto end = GetPipe().writeQueue.begin();
            if (GetPipe().writeQueue.size() > 1000)
            {
                for (int i = 0; i < 1000; i++)
                {
                    end++;
                }
            }
            else
            {
                end = GetPipe().writeQueue.end();
            }
            writeQueueThread.splice(writeQueueThread.begin(), GetPipe().writeQueue, GetPipe().writeQueue.begin(), end);
            //GetPipe().writeQueue.swap(writeQueueThread);
        }
    }

    long long int counter2 = 0;
    auto it = writeQueueThread.begin();

    writeCounter += writeQueueThread.size();
    for (; it != writeQueueThread.end(); it++)
    {
        IPacket* packet = *it;
        /*SOCKADDR_IN dst;
        dst.sin_family = AF_INET;
        inet_pton(AF_INET, "127.0.0.1", &(dst.sin_addr));
        inet_pton(AF_INET, "192.168.1.1", &(dst.sin_addr));
        dst.sin_port = htons(7777);*/

        //while (true)
        {
            counter++;
            counter2++;
            //WSABUF buffers[2];

            //buffers[0].buf = packet->GetBuffer().data();
            //buffers[0].len = packet->GetBuffer().size();
            //buffers[1].buf = packet->buffer.data();
            //buffers[1].len = packet->buffer.size();

            DWORD send = 0;
            //int size = buffers[0].len + buffers[1].len;
            //auto result2 = WSASendTo(listenSocket, buffers, 1, &send, 0, (PSOCKADDR)& dst, sizeof(dst), nullptr, nullptr);
            //int result = sendto(GetPipe().listenSocket, packet->GetBuffer().data(), packet->GetBuffer().size(), 0, (PSOCKADDR)& dst, sizeof(dst));
            int result = sendto(GetPipe().listenSocket, packet->GetBuffer().data(), packet->GetBuffer().size(), 0, (PSOCKADDR)& (packet->GetDestination()), sizeof(SOCKADDR_IN));
            if (result != packet->GetBuffer().size())
            {
                Sleep(1);
                counterError++;
                break;
            }
        }
    }

    if (!writeQueueThread.empty())
    {
        std::lock_guard<std::mutex> lock(GetPipe().freeWriteQueueMutex);
        GetPipe().freeWriteQueue.splice(GetPipe().freeWriteQueue.end(), writeQueueThread, writeQueueThread.begin(), it);

        SetEvent(GetPipe().writeEvent);
    }

    SetEvent(GetPipe().writeEvent);

    /*if (!writeQueueThread.empty())
    {
        //std::lock_guard<std::mutex> lock(GetPipe().writeQueueMutex);
        writeQueueThread.splice(writeQueueThread.begin(), packetsToSend);
        SetEvent(GetPipe().writeEvent);
    }*/
}