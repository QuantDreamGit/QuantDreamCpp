#include "request/contracts/ContractDetails.h"
#include "Order.h"
#include "contracts/StockContracts.h"
#include "helpers/connection.h"
#include "orders/common_orders.h"
#include "orders/management/open.h"
#include "orders/options/simple_order.h"
#include "request/options/chain.h"
#include "wrappers/IBWrapperBase.h"

int main() {
  // Logger setup
  Logger::setEnabled(true);
  Logger::setLevel(Logger::Level::TIMER);

  // IB Wrapper setup
  IBWrapperBase ib;
  std::string exchange = "CBOE";
  // Connect to IB Gateway or TWS
  IB::Helpers::ensureConnected(ib, "127.0.0.1", 4002, 0);
  // Set market data type to real-time
  ib.client->reqMarketDataType(1);

  // Get the underlying contract
  Contract const underlying = IB::Contracts::makeStock("GOOGL", exchange, "USD");

  // Get chain info
  const IB::Options::ChainInfo optChain = IB::Request::getOptionChain(ib,
                                                                   underlying,
                                                                   IB::ReqId::OPTION_CHAIN_ID,
                                                                   0.1,
                                                                   exchange);

  // Create a market order
  const Order marketOrder = IB::Orders::MarketBuy(1);

  // Execute simple option order
  IB::Orders::Options::placeSimpleOrder(ib, underlying, optChain, marketOrder, "C");

  // Check if an order is currently open
  // Since it's not possible to be sure when open orders are finished a get function is required to
  // fetch them from Wrapper buffer.
  // Furthermore, when requesting order the buffer is always updated
  IB::Orders::Management::Open::requestClientOpenOrders(ib);
  auto openOrders = ib.getOpenOrders();

  // Cancel all orders
  // IB::Orders::Management::Open::cancelAll(ib);

  // auto callGreeks = IB::Requests::getGreeksTable(ib, underlying, optChain, "C");

  std::cout << "Press Enter to exit..." << std::endl;
  std::cin.get();

  ib.disconnect();
  return 0;
}

