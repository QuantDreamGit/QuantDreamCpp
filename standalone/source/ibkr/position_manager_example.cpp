/**
 * @file position_manager_example.cpp
 * @brief Example demonstrating how to use PositionManager with market data callbacks
 *
 * This example now uses an event-driven architecture:
 * - No blocking sleep_for()
 * - Main loop waits for new events using std::condition_variable
 * - Strategy logic executes as callbacks arrive in real-time
 */

#include <chrono>
#include <iostream>
#include <thread>
#include <cmath>
#include <atomic>
#include <condition_variable>
#include <csignal>

#include "contracts/StockContracts.h"
#include "helpers/connection.h"
#include "orders/common_orders.h"
#include "orders/management/position.h"
#include "orders/options/condor_order.h"
#include "request/market_data/market_data.h"
#include "request/options/chain.h"
#include "strategy/position_manager.h"
#include "wrappers/IBStrategyWrapper.h"

// =============================================================================
// Example Strategy Class Using PositionManager
// =============================================================================
class ExampleStrategy {
public:
  ExampleStrategy(PositionManager& pm, IBStrategyWrapper& ibWrapper)
    : positionManager_(pm), ibWrapper_(ibWrapper) {
    setupCallbacks();
  }

  void setupCallbacks() {
    // Callback for position updates
    positionManager_.setOnPositionCallback([this](const IB::Accounts::PositionInfo& position) {
      std::cout << "[Strategy] Position detected: " << position.contract.symbol
                << " Qty: " << position.position
                << " @ " << position.avgCost << std::endl;
      onPositionDetected(position);
      notifyEvent_();
    });

    // Market data callbacks
    positionManager_.setOnBidCallback([this](int tickerId, double bid) {
      onBidUpdate(tickerId, bid);
      notifyEvent_();
    });

    positionManager_.setOnAskCallback([this](int tickerId, double ask) {
      onAskUpdate(tickerId, ask);
      notifyEvent_();
    });

    positionManager_.setOnMidCallback([this](int tickerId, double mid) {
      onMidUpdate(tickerId, mid);
      notifyEvent_();
    });

    positionManager_.setOnLastCallback([this](int tickerId, double last) {
      onLastUpdate(tickerId, last);
      notifyEvent_();
    });

    positionManager_.setOnSnapshotCallback([this](int tickerId, const IB::MarketData::MarketSnapshot& snapshot) {
      onSnapshotReady(tickerId, snapshot);
      notifyEvent_();
    });
  }

  void onBidUpdate(int tickerId, double bid) {
    bestBid_[tickerId] = bid;
    std::cout << "[Strategy] Bid update: " << bid << std::endl;
  }

  void onAskUpdate(int tickerId, double ask) {
    bestAsk_[tickerId] = ask;
    if (bestBid_.count(tickerId)) {
      double spread = bestAsk_[tickerId] - bestBid_[tickerId];
      std::cout << "[Strategy] Spread: " << spread << std::endl;
    }
  }

  void onMidUpdate(int tickerId, double mid) {
    fairValue_[tickerId] = mid;
    std::cout << "[Strategy] Mid price: " << mid << std::endl;
  }

  void onLastUpdate(int tickerId, double last) {
    priceHistory_[tickerId].push_back(last);
    if (priceHistory_[tickerId].size() > 10) {
      double oldPrice = priceHistory_[tickerId][priceHistory_[tickerId].size() - 10];
      double momentum = (last - oldPrice) / oldPrice * 100.0;
      std::cout << "[Strategy] Momentum: " << momentum << "%" << std::endl;
    }
  }

  void onSnapshotReady(int tickerId, const IB::MarketData::MarketSnapshot& snapshot) {
    std::cout << "[Strategy] Snapshot for " << tickerId
              << " | Bid: " << snapshot.bid
              << " Ask: " << snapshot.ask
              << " Last: " << snapshot.last << std::endl;
  }

  void onPositionDetected(const IB::Accounts::PositionInfo& position) {
    std::cout << "[Strategy] Auto-closing all positions." << std::endl;
    IB::Orders::Management::closeAllPositions(ibWrapper_);
  }

  void printCurrentPositions() {
    auto positions = positionManager_.snapshot();
    std::cout << "\n[Strategy] Current Positions (" << positions.size() << "):" << std::endl;
    for (const auto& pos : positions) {
      std::cout << "  " << pos.contract.symbol
                << " " << pos.contract.secType
                << ": " << pos.position
                << " @ " << pos.avgCost << std::endl;
    }
    std::cout << std::endl;
  }

  // Register notifier from main thread
  void setEventNotifier(std::function<void()> fn) { notifyEvent_ = std::move(fn); }

private:
  PositionManager& positionManager_;
  IBStrategyWrapper& ibWrapper_;
  std::function<void()> notifyEvent_;
  std::map<int, double> bestBid_, bestAsk_, fairValue_;
  std::map<int, std::vector<double>> priceHistory_;
};

// =============================================================================
// Event-driven Main
// =============================================================================
std::atomic<bool> terminateFlag{false};
std::mutex eventMutex;
std::condition_variable eventCV;
std::atomic<bool> eventReceived{false};

void signalHandler(int signal) {
  if (signal == SIGINT) {
    terminateFlag = true;
    eventCV.notify_all();
  }
}

int main() {
  Logger::setEnabled(true);
  Logger::setLevel(Logger::Level::TIMER);
  std::signal(SIGINT, signalHandler);

  std::cout << "\n=== PositionManager Event-Driven Example ===\n" << std::endl;

  IBStrategyWrapper ib;
  IB::Helpers::ensureConnected(ib, "127.0.0.1", 4002, 0);
  std::cout << "[Main] Connected to IB Gateway" << std::endl;

  PositionManager positionManager;
  ib.setPositionManager(&positionManager);
  std::cout << "[Main] PositionManager wired to IBStrategyWrapper" << std::endl;

  ExampleStrategy strategy(positionManager, ib);
  std::cout << "[Main] Strategy initialized\n" << std::endl;

  // Register notifier
  strategy.setEventNotifier([&]() {
    {
      std::lock_guard<std::mutex> lock(eventMutex);
      eventReceived = true;
    }
    eventCV.notify_one();
  });

  ib.client->reqPositions();
  std::cout << "[Main] Waiting for IB events... Press Ctrl+C to stop.\n" << std::endl;

  // Main event-driven loop
  while (!terminateFlag) {
    std::unique_lock<std::mutex> lock(eventMutex);
    eventCV.wait_for(lock, std::chrono::seconds(1), [&] {
      return eventReceived.load() || terminateFlag.load();
    });

    if (terminateFlag) break;
    if (eventReceived) {
      eventReceived = false;
      // Optional: perform background checks (PnL, risk, etc.)
    }
  }

  std::cout << "\n[Main] Disconnecting..." << std::endl;
  ib.disconnect();
  std::cout << "=== Example Complete ===" << std::endl;
  return 0;
}
