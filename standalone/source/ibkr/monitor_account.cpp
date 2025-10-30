#include "wrappers/IBWrapperMonitor.h"
#include <thread>
#include <chrono>

int main() {
  // Logger setup
  Logger::setEnabled(true);
  Logger::setLevel(Logger::Level::INFO);

  IBWrapperMonitor monitor;

  if (!monitor.connect("127.0.0.1", 4002, 2))
    return 1;

  // Wait for IB to send nextValidOrderId confirmation
  while (monitor.nextValidOrderId <= 0)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // ✅ No need to poll manually — monitor already does it internally
  LOG_INFO("[Main] Monitoring orders... Press Ctrl+C to exit.");

  // Keep running
  while (true)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  monitor.disconnect();
  return 0;
}
