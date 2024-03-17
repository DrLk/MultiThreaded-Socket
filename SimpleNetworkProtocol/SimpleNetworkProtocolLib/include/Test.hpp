#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <stop_token>
#include <thread>
#include <utility>
#include <vector>

#include "Connection.hpp"
#include "ConnectionAddr.hpp"
#include "FastTransportProtocol.hpp"
#include "HeaderTypes.hpp"
#include "IPacket.hpp"
#include "IRecvQueue.hpp"
#include "ISpeedControllerState.hpp"
#include "LockedList.hpp"
#include "Logger.hpp"
#include "Packet.hpp"
#include "PeriodicExecutor.hpp"
#include "RecvQueue.hpp"
#include "SampleStats.hpp"
#include "Socket.hpp"
#include "TimeRangedStats.hpp"
#include "UDPQueue.hpp"

namespace FastTransport::Protocol {
void TestConnection();

} // namespace FastTransport::Protocol
