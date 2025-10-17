#include "../../../external/IBWrapper/include/request/contracts/ContractDetails.h"
#include "../../../external/IBWrapper/include/request/market_data/MarketDataRequests.h"
#include "contracts/StockContracts.h"
#include "request/options/OptionChain.h"
#include "wrappers/IBWrapperBase.h"

int main() {
  IBWrapperBase ib;

  if (!ib.connect()) { return 1; }

  IB::Options::ChainInfo optChain = IB::Request::getOptionChain(ib, IB::Contracts::makeStock("AAPL"));

  std::cout << "Press Enter to exit..." << std::endl;
  std::cin.get();

  ib.disconnect();
  return 0;
}

