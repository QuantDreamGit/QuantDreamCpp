#include "wrappers/IBWrapperBase.h"
#include "contracts/StockContracts.h"
#include "request/MarketDataRequests.h"

int main() {
  IBWrapperBase ib;

  if (!ib.connect()) { return 1; }

  // Create a stock contract for GOOG
  // Contract stock = IB::Contracts::makeStock("GOOGL");
  // IB::Requests::requestMarketData(ib.client, 3, stock);

  // Create an option contract for GOOGL 20251215 150 Call
  // Contract option = IB::Contracts::makeOption("GOOGL", "20251015", 250.0, "C");
  // IB::Requests::requestMarketData(ib.client, 3, option, 2001);

  ib.client -> reqContractDetails(999, IB::Contracts::makeStock("AAPL"));
  ib.client -> reqSecDefOptParams(9001, "AAPL", "", "STK", 265598);
  std::cout << "Press Enter to exit..." << std::endl;
  std::cin.get();

  ib.disconnect();
  return 0;
}

