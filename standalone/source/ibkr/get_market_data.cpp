/**
 * @file get_market_data.cpp
 * @brief IB API testing utility with toggleable feature sections
 *
 * Toggle features via FeatureFlags namespace. Each section is independent.
 * Connection: IB Gateway/TWS on 127.0.0.1:4002
 */

#include "contracts/StockContracts.h"
#include "helpers/connection.h"
#include "helpers/open_markets.h"
#include "orders/common_orders.h"
#include "orders/management/open.h"
#include "orders/management/pnl.h"
#include "orders/management/position.h"
#include "orders/options/condor_order.h"
#include "orders/options/simple_order.h"
#include "request/options/chain.h"
#include "wrappers/IBBaseWrapper.h"
#include "wrappers/IBStrategyWrapper.h"

// =============================================================================
// Feature Flags - Enable/disable sections
// =============================================================================
namespace FeatureFlags {
  constexpr bool OPTION_CHAIN = true;       ///< Fetch option chain
  constexpr bool SIMPLE_ORDER = false;      ///< Place simple option order
  constexpr bool IRON_CONDOR = false;        ///< Execute iron condor
  constexpr bool ACCOUNT_SUMMARY = false;   ///< Request account data
  constexpr bool POSITION_MGT = true;      ///< Query/close positions
  constexpr bool PNL_MONITOR = false;       ///< Real-time P&L loop
  constexpr bool CANCEL_ORDERS = false;     ///< Cancel all orders
}

// =============================================================================
// Main
// =============================================================================
int main() {
  // Setup
  Logger::setEnabled(true);
  Logger::setLevel(Logger::Level::TIMER);
  const std::string exchange = "SMART";

  IBStrategyWrapper ib;
  IB::Helpers::ensureConnected(ib, "127.0.0.1", 4002, 0);
  Contract const underlying = IB::Contracts::makeStock("GOOGL", exchange, "USD");

  // Section 1: Option Chain
  IB::Options::ChainInfo optChain;
  if (FeatureFlags::OPTION_CHAIN) {
    LOG_INFO("=== Fetching Option Chain ===");
    optChain = IB::Request::getOptionChain(ib, underlying, IB::ReqId::OPTION_CHAIN_ID, 0.1, exchange);
    LOG_INFO("Found ", optChain.expirations.size(), " expirations\n");
  }

  // Section 2: Simple Order
  if (FeatureFlags::SIMPLE_ORDER) {
    LOG_INFO("=== Placing Simple Order ===");
    const Order marketOrder = IB::Orders::MarketBuy(1);
    IB::Orders::Options::placeSimpleOrder(ib, underlying, optChain, marketOrder, "C");
    LOG_INFO("Order submitted\n");
  }

  // Section 3: Iron Condor
  if (FeatureFlags::IRON_CONDOR) {
    LOG_INFO("=== Executing Iron Condor ===");
    IB::Orders::Options::placeIronCondor(ib, underlying, optChain, *optChain.expirations.begin(),
                                         {}, 1, true, 0.1, true);
    LOG_INFO("Condor submitted\n");
  }

  // Section 4: Account Summary
  if (FeatureFlags::ACCOUNT_SUMMARY) {
    LOG_INFO("=== Requesting Account Summary ===");
    ib.client->reqAccountSummary(9001, "All", "NetLiquidation,TotalCashValue,BuyingPower,AvailableFunds");
    LOG_INFO("Request sent\n");
  }

  // Section 5: Position Management
  if (FeatureFlags::POSITION_MGT) {
    LOG_INFO("=== Managing Positions ===");
    auto positions = IB::Orders::Management::getOpenPositions(ib);
    LOG_INFO("Found ", positions.size(), " positions");
    IB::Orders::Management::closeAllPositions(ib);
    LOG_INFO("Close orders submitted\n");
  }

  // Section 6: P&L Monitoring (blocks until Ctrl+C)
  if (FeatureFlags::PNL_MONITOR) {
    LOG_INFO("=== Starting P&L Monitor (5s refresh) ===");
    while (true) {
      try {
        IB::Orders::Management::showCurrentPnL(ib);
      } catch (const std::exception& e) {
        LOG_ERROR("P&L error: ", e.what());
      }
      std::this_thread::sleep_for(std::chrono::seconds(5));
    }
  }

  // Section 7: Cancel All Orders
  if (FeatureFlags::CANCEL_ORDERS) {
    LOG_INFO("=== Cancelling All Orders ===");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    IB::Orders::Management::Open::cancelAll(ib);
    LOG_INFO("Cancellation sent\n");
  }

  // Cleanup
  std::cout << "\nPress Enter to exit..." << std::endl;
  std::cin.get();
  ib.disconnect();
  LOG_INFO("Disconnected");

  return 0;
}
