/**
 * @file position_manager_example.cpp
 * @brief Example demonstrating how to use PositionManager with market data callbacks
 *
 * This example shows how to:
 * 1. Create a PositionManager instance
 * 2. Register callbacks for market data events (bid, ask, mid, last, snapshot)
 * 3. Wire the PositionManager to IBStrategyWrapper
 * 4. Handle market data updates in your strategy
 *
 * The PositionManager acts as a bridge between IB market data events and your
 * trading strategy, providing real-time price updates and position tracking.
 */

#include <chrono>
#include <iostream>
#include <thread>
#include <cmath>

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

/**
 * @brief Example trading strategy that uses PositionManager for market data
 *
 * This strategy demonstrates how to:
 * - Register callbacks on PositionManager to receive market data updates
 * - Track current positions
 * - React to price changes in real-time
 */
class ExampleStrategy {
public:
  ExampleStrategy(PositionManager& pm, IBStrategyWrapper& ibWrapper) 
    : positionManager_(pm), ibWrapper_(ibWrapper) {
    setupCallbacks();
  }

  /**
   * @brief Register all market data callbacks on the PositionManager
   *
   * This is where you wire your strategy logic to respond to market data events.
   * Each callback receives a tickerId (the IB request ID) and the price data.
   */
  void setupCallbacks() {
    // Callback for position updates - automatically close any position detected
    positionManager_.setOnPositionCallback([this](const IB::Accounts::PositionInfo& position) {
      std::cout << "[Strategy] Position detected: " << position.contract.symbol 
                << " " << position.contract.secType
                << " Qty: " << position.position 
                << " @ " << position.avgCost << std::endl;
      
      // Custom logic: automatically close this position
      onPositionDetected(position);
    });

    // Callback for bid price updates
    positionManager_.setOnBidCallback([this](int tickerId, double bid) {
      std::cout << "[Strategy] Bid update for tickerId " << tickerId 
                << ": " << bid << std::endl;
      // Your strategy logic here - e.g., check if bid is attractive for buying
      onBidUpdate(tickerId, bid);
    });

    // Callback for ask price updates
    positionManager_.setOnAskCallback([this](int tickerId, double ask) {
      std::cout << "[Strategy] Ask update for tickerId " << tickerId 
                << ": " << ask << std::endl;
      // Your strategy logic here - e.g., check if ask is attractive for selling
      onAskUpdate(tickerId, ask);
    });

    // Callback for mid price (average of bid/ask)
    positionManager_.setOnMidCallback([this](int tickerId, double mid) {
      std::cout << "[Strategy] Mid price for tickerId " << tickerId 
                << ": " << mid << std::endl;
      // Your strategy logic here - e.g., update fair value estimate
      onMidUpdate(tickerId, mid);
    });

    // Callback for last trade price
    positionManager_.setOnLastCallback([this](int tickerId, double last) {
      std::cout << "[Strategy] Last price for tickerId " << tickerId 
                << ": " << last << std::endl;
      // Your strategy logic here - e.g., update momentum indicators
      onLastUpdate(tickerId, last);
    });

    // Callback for complete market snapshot (all data ready)
    positionManager_.setOnSnapshotCallback([this](int tickerId, 
                                                   const IB::MarketData::MarketSnapshot& snapshot) {
      std::cout << "[Strategy] Complete snapshot for tickerId " << tickerId << std::endl;
      std::cout << "  Bid: " << snapshot.bid << ", Ask: " << snapshot.ask 
                << ", Last: " << snapshot.last << std::endl;
      if (snapshot.hasGreeks) {
        std::cout << "  Delta: " << snapshot.delta << ", IV: " << snapshot.impliedVol << std::endl;
      }
      // Your strategy logic here - e.g., make trading decision based on complete data
      onSnapshotReady(tickerId, snapshot);
    });
  }

  /**
   * @brief Example strategy logic for bid updates
   */
  void onBidUpdate(int tickerId, double bid) {
    // Example: Track best bid
    bestBid_[tickerId] = bid;
    
    // Example: Check if bid crosses a threshold
    if (bid > 150.0) {
      std::cout << "[Strategy] Bid crossed threshold! Consider selling." << std::endl;
    }
  }

  /**
   * @brief Example strategy logic for ask updates
   */
  void onAskUpdate(int tickerId, double ask) {
    // Example: Track best ask
    bestAsk_[tickerId] = ask;
    
    // Example: Check spread
    if (bestBid_.count(tickerId) && bestAsk_.count(tickerId)) {
      double spread = bestAsk_[tickerId] - bestBid_[tickerId];
      std::cout << "[Strategy] Spread: " << spread << std::endl;
    }
  }

  /**
   * @brief Example strategy logic for mid price updates
   */
  void onMidUpdate(int tickerId, double mid) {
    // Example: Use mid price for fair value calculations
    fairValue_[tickerId] = mid;
    
    // Example: Compare with your model's fair value
    // double modelValue = getModelValue(tickerId);
    // if (mid < modelValue * 0.95) {
    //     std::cout << "[Strategy] Undervalued! Consider buying." << std::endl;
    // }
  }

  /**
   * @brief Example strategy logic for last trade updates
   */
  void onLastUpdate(int tickerId, double last) {
    // Example: Track price history for momentum
    priceHistory_[tickerId].push_back(last);
    if (priceHistory_[tickerId].size() > 100) {
      priceHistory_[tickerId].erase(priceHistory_[tickerId].begin());
    }
    
    // Example: Simple momentum check
    if (priceHistory_[tickerId].size() >= 10) {
      double oldPrice = priceHistory_[tickerId][priceHistory_[tickerId].size() - 10];
      double momentum = (last - oldPrice) / oldPrice * 100.0;
      std::cout << "[Strategy] 10-tick momentum: " << momentum << "%" << std::endl;
    }
  }

  /**
   * @brief Example strategy logic when complete snapshot is ready
   */
  void onSnapshotReady(int tickerId, const IB::MarketData::MarketSnapshot& snapshot) {
    // Example: Make trading decision based on complete data
    
    // Check if we have positions
    auto positions = positionManager_.snapshot();
    bool hasPosition = false;
    for (const auto& pos : positions) {
      if (pos.contract.conId == tickerId) {
        hasPosition = true;
        std::cout << "[Strategy] Current position: " << pos.position 
                  << " @ avg cost " << pos.avgCost << std::endl;
      }
    }
    
    // Example decision logic
    if (!hasPosition && snapshot.hasBidAsk()) {
      double mid = (snapshot.bid + snapshot.ask) / 2.0;
      std::cout << "[Strategy] No position. Mid price: " << mid << std::endl;
      // Add your entry logic here
    }
    
    // Example: Options strategy using Greeks
    if (snapshot.hasGreeksData()) {
      std::cout << "[Strategy] Option Greeks available:" << std::endl;
      std::cout << "  Delta: " << snapshot.delta << " (hedge ratio)" << std::endl;
      std::cout << "  Gamma: " << snapshot.gamma << " (delta sensitivity)" << std::endl;
      std::cout << "  Vega: " << snapshot.vega << " (IV sensitivity)" << std::endl;
      std::cout << "  Theta: " << snapshot.theta << " (time decay)" << std::endl;
      
      // Example: Delta-neutral strategy
      // if (abs(totalDelta) > 0.1) {
      //     hedgeWithUnderlying(snapshot.delta);
      // }
    }
  }

  /**
   * @brief Get current positions from the PositionManager
   */
  void printCurrentPositions() {
    auto positions = positionManager_.snapshot();
    std::cout << "\n[Strategy] Current Positions (" << positions.size() << "):" << std::endl;
    for (const auto& pos : positions) {
      std::cout << "  " << pos.contract.symbol << " " << pos.contract.secType
                << ": " << pos.position << " @ " << pos.avgCost << std::endl;
    }
    std::cout << std::endl;
  }

  /**
   * @brief Called when a position is detected - automatically closes it
   *
   * This demonstrates how to use the position callback to implement
   * automatic position management (e.g., auto-closing unwanted positions).
   */
  void onPositionDetected(const IB::Accounts::PositionInfo& position) {
    IB::Orders::Management::closeAllPositions(ibWrapper_);
  }

private:
  PositionManager& positionManager_;
  IBStrategyWrapper& ibWrapper_;  // Reference to IB wrapper for placing orders
  
  // Strategy state tracking
  std::map<int, double> bestBid_;
  std::map<int, double> bestAsk_;
  std::map<int, double> fairValue_;
  std::map<int, std::vector<double>> priceHistory_;
};

// =============================================================================
// Main Example Program
// =============================================================================

int main() {
  Logger::setEnabled(true);
  Logger::setLevel(Logger::Level::TIMER);

  std::cout << "\n=== PositionManager Market Data Callback Example ===\n" << std::endl;

  // Step 1: Create IBStrategyWrapper (combines market data + account + orders)
  IBStrategyWrapper ib;
  
  // Step 2: Connect to IB Gateway/TWS
  IB::Helpers::ensureConnected(ib, "127.0.0.1", 4002, 0);
  std::cout << "[Main] Connected to IB Gateway" << std::endl;

  // Step 3: Create PositionManager
  PositionManager positionManager;
  std::cout << "[Main] PositionManager created" << std::endl;

  // Step 4: Wire PositionManager to both IBAccountWrapper and IBMarketWrapper
  // This allows the PositionManager to receive both position updates and market data
  ib.setPositionManager(&positionManager);
  std::cout << "[Main] PositionManager wired to IBStrategyWrapper" << std::endl;

  // Step 5: Create your strategy and register callbacks (pass ib reference for order placement)
  ExampleStrategy strategy(positionManager, ib);
  std::cout << "[Main] Strategy created with callbacks registered\n" << std::endl;

  // Step 6: Request current positions to trigger auto-close on any existing positions
  std::cout << "[Main] Requesting current positions..." << std::endl;
  ib.client->reqPositions();
  std::this_thread::sleep_for(std::chrono::seconds(2));
  strategy.printCurrentPositions();

  // Step 7: Create stock contract for market data requests
  Contract const underlying = IB::Contracts::makeStock("GOOGL", "SMART", "USD");

  // Request option chain
  LOG_INFO("=== Fetching Option Chain ===");
  IB::Options::ChainInfo optChain = IB::Request::getOptionChain(ib,
                                                                   underlying, IB::ReqId::OPTION_CHAIN_ID,
                                                                   0.1,
                                                                   "SMART");
  LOG_INFO("Found ", optChain.expirations.size(), " expirations\n");

  // Executing Iron Condor as an example order
  LOG_INFO("=== Executing Iron Condor ===");
  IB::Orders::Options::placeIronCondor(ib, underlying, optChain, *optChain.expirations.begin(),
                                       {}, 1, true, 0.1, true);
  LOG_INFO("Condor submitted\n");

  // Wait to receive market data and process callbacks
  std::cout << "[Main] Running strategy for 50 seconds to receive market data..." << std::endl;
  // std::this_thread::sleep_for(std::chrono::seconds(500));

  
  std::cout << "[Main] Final positions:" << std::endl;
  strategy.printCurrentPositions();

  std::cout << "[Main] Disconnecting..." << std::endl;
  ib.disconnect();
  
  std::cout << "\n=== Example Complete ===\n" << std::endl;

  return 0;
}
