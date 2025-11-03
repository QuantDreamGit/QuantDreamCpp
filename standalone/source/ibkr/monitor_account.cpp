/**
 * @file monitor_positions_pnl.cpp
 * @brief Continuously monitor open positions and P&L (press Ctrl+C to stop)
 *
 * Connects to IB Gateway/TWS on 127.0.0.1:4002 and every \c REFRESH_SECONDS
 * seconds prints the number of open positions and invokes the existing
 * PnL display routine.
 */

#include "contracts/StockContracts.h"
#include "helpers/connection.h"
#include "orders/management/position.h"
#include "orders/management/pnl.h"
#include "wrappers/IBStrategyWrapper.h"

#include <atomic>
#include <csignal>
#include <chrono>
#include <thread>
#include <iostream>

namespace {
  constexpr int REFRESH_SECONDS = 5;
  std::atomic_bool g_running{true};

  void handle_sigint(int) {
    g_running.store(false);
  }
}

int main() {
  // Setup
  Logger::setEnabled(true);
  Logger::setLevel(Logger::Level::TIMER);

  std::signal(SIGINT, handle_sigint);

  IBStrategyWrapper ib;
  IB::Helpers::ensureConnected(ib, "127.0.0.1", 4002, 5);

  LOG_INFO("=== Position & PnL Monitor started (Ctrl+C to stop) ===");

  while (g_running.load()) {
    try {
      auto positions = IB::Orders::Management::getOpenPositions(ib);
      LOG_INFO("Open positions: ", positions.size());

      // Reuse existing PnL display function (prints to logs/console)
      IB::Orders::Management::showCurrentPnL(ib);
    } catch (const std::exception& e) {
      LOG_ERROR("Monitor error: ", e.what());
    }

    for (int i = 0; i < REFRESH_SECONDS && g_running.load(); ++i) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  LOG_INFO("Stopping monitor, disconnecting...");
  ib.disconnect();
  LOG_INFO("Disconnected. Exiting.");

  return 0;
}
