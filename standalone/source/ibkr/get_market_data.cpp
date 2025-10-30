#include "contracts/StockContracts.h"
#include "helpers/connection.h"
#include "helpers/open_markets.h"
#include "orders/management/open.h"
#include "orders/management/pnl.h"
#include "orders/management/position.h"
#include "orders/options/condor_order.h"
#include "request/options/chain.h"
#include "wrappers/IBBaseWrapper.h"
#include "wrappers/IBStrategyWrapper.h"

int main() {
  // Logger setup
  Logger::setEnabled(true);
  Logger::setLevel(Logger::Level::TIMER);

  // Constants
  const std::string exchange = "SMART";

  // IB Wrapper setup
  IBStrategyWrapper ib;
  // Connect to IB Gateway or TWS (by default it uses realtime market data)
  IB::Helpers::ensureConnected(ib, "127.0.0.1", 4002, 0);

  // Get the underlying contract
  Contract const underlying = IB::Contracts::makeStock("GOOGL", exchange, "USD");

  // Get chain info
  const IB::Options::ChainInfo optChain = IB::Request::getOptionChain(ib,
                                                                   underlying,
                                                                   IB::ReqId::OPTION_CHAIN_ID,
                                                                   0.1,
                                                                   exchange);

  // Create a market order
  // const Order marketOrder = IB::Orders::MarketBuy(1);
  // Execute simple option order
  // IB::Orders::Options::placeSimpleOrder(ib, underlying, optChain, marketOrder, "C");

  // Cancel all orders
  // IB::Orders::Management::Open::cancelAll(ib);

  /*
  IB::Orders::Options::placeIronCondor(
      ib,
      underlying,
      optChain,
      *optChain.expirations.begin(),
      {},                             // no strikes -> auto-selects middle 4
      1,
      true,                             // buy condor
      0.1,
      true                              // Confirms auto-strikes usage
  );
  */

  // ib.client->reqAccountSummary(
  //   9001,           // reqId (any unique integer)
  //   "All",          // group ("All" = all accounts)
  //   "NetLiquidation,TotalCashValue,BuyingPower,AvailableFunds"
  // );

  // auto positions = IB::Orders::Management::getOpenPositions(ib);
  // IB::Orders::Management::closeAllPositions(ib);
  // IB::Orders::Management::showCurrentPnL(ib);

  IB::Orders::Management::closeAllPositions(ib);
  /*
  while (true) {
    try {
      IB::Orders::Management::showCurrentPnL(ib);
    } catch (const std::exception& e) {
      LOG_ERROR("[PnLLoop] Exception: ", e.what());
    }

    // Sleep for 1 second between refreshes
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }
  IB::Orders::Management::closeAllPositions(ib);
  */
  // ib.disconnect();
  // Cancel all orders after waiting for one second
  // std::this_thread::sleep_for(std::chrono::seconds(1));
  // IB::Orders::Management::Open::cancelAll(ib);



  // Execute a condor strategy
  // IB::Orders::Options::placeIronCondor(ib, underlying, optChain, *optChain.expirations.begin(), {125.0, 130.0, 140.0, 145.0}, 1, true);

  // Check if an order is currently open
  // Since it's not possible to be sure when open orders are finished a get function is required to
  // fetch them from Wrapper buffer.
  // Furthermore, when requesting order the buffer is always updated
  // IB::Orders::Management::Open::requestClientOpenOrders(ib);
  // auto openOrders = ib.getOpenOrders();



  // auto callGreeks = IB::Requests::getGreeksTable(ib, underlying, optChain, "C");

  std::cout << "Press Enter to exit..." << std::endl;
  std::cin.get();

  ib.disconnect();
  return 0;
}
