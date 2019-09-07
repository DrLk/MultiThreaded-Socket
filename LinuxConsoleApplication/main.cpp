#include <cstdio>
#include <unistd.h>

#include "LinuxUDPSocket.h"




int main(void)
{
    LinuxUDPSocket linuxSocket;
    
    linuxSocket.Init();
    sleep(10000);
    return 0;
}