#include "external/IBWrapper/test_strategy.h"

#include <chrono>
#include <csignal>
#include <memory>
#include <thread>

#include "contracts/OptionContract.h"
#include "strategy/engine.h"
#include "strategy/order_execution.h"
#include "strategy/queue.h"

/**
 * @file main.cpp
 * @brief Demonstrates how to run a simple strategy using the trading framework.
 *
 * This example creates a shared order queue, starts a single strategy instance,
 * feeds it with mock market data, and then stops it after a short delay.
 *
 * It serves as a minimal test harness for validating that:
 *  - The strategy can receive and process market snapshots.
 *  - The strategy issues order requests correctly.
 *  - The strategy can start and stop gracefully.
 *
 * In a real application, this `main` function would also:
 *  - Initialize the IB API connection.
 *  - Launch an OrderExecutor to consume the outgoing orders.
 *  - Connect to the PositionManager for live position tracking.
 */

/**
 * @brief Entry point of the trading test program.
 *
 * The function initializes a concurrent queue for outgoing orders, starts
 * a simple trading strategy, sends it mock market data (with `last=100`),
 * and then shuts down after three seconds.
 *
 * @return Exit status code (0 on success).
 */
int main() {
  /// Create a shared concurrent queue for outgoing order requests.
  auto orderQueue = std::make_shared<ConcurrentQueue<OrderRequest>>();

  /// Instantiate and start the simple test strategy.
  SimpleStrategy strat(orderQueue);
  strat.start();

  /// Create a fake market snapshot with last price > 0 to trigger a buy signal.
  MarketSnapshot snap;
  snap.last = 100.0;
  strat.onSnapshot(snap);  ///< Send snapshot to strategy (simulates market tick).

  /// Let the strategy run briefly to demonstrate its behavior.
  std::this_thread::sleep_for(std::chrono::seconds(3));

  /// Stop the strategy gracefully.
  strat.stop();

  return 0;
}
