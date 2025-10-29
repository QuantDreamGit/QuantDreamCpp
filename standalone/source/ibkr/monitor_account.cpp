#include "wrappers/IBWrapperMonitor.h"
#include <thread>
#include <chrono>

int main() {
  IBWrapperMonitor monitor;

  if (!monitor.connect("127.0.0.1", 4002, 2))
    return 1;

  while (monitor.nextValidOrderId <= 0)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  while (true) {
    monitor.client->reqAllOpenOrders();
    std::this_thread::sleep_for(std::chrono::seconds(2));
  }

  monitor.disconnect();
  return 0;
}