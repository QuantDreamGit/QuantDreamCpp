#ifndef QUANTDREAMCPP_TEST_STRATEGY_H
#define QUANTDREAMCPP_TEST_STRATEGY_H

#include <atomic>
#include <memory>
#include <thread>
#include <chrono>
#include "strategy/strategy_base.h"
#include "strategy/order_execution.h"
#include "strategy/queue.h"

/**
 * @brief A minimal example trading strategy.
 *
 * The SimpleStrategy demonstrates how to:
 *  - React to market data snapshots.
 *  - Send an order when a condition is met (`price > 0`).
 *  - Avoid sending new orders while one is still active.
 *  - Mark the order as "closed" after simulated execution.
 *
 * In a real implementation, the order status would be tracked using
 * broker callbacks (e.g., IB's `orderStatus()`), and positions would
 * be updated from a `PositionManager`.
 */
class SimpleStrategy : public StrategyBase {
public:
  /**
   * @brief Construct a SimpleStrategy instance.
   *
   * @param outQueue Shared pointer to the order queue used to send order requests.
   */
  explicit SimpleStrategy(std::shared_ptr<ConcurrentQueue<OrderRequest>> outQueue)
    : outQueue_(std::move(outQueue)), running_(false), orderActive_(false)
  {}

  /**
   * @brief Start the strategy execution thread.
   *
   * Launches a worker thread that periodically checks for new market data
   * and runs the trading logic.
   */
  void start() override {
    running_ = true;
    worker_ = std::thread([this]{ loop(); });
  }

  /**
   * @brief Stop the strategy gracefully.
   *
   * Signals the background thread to stop and waits for it to join.
   */
  void stop() override {
    running_ = false;
    if (worker_.joinable()) worker_.join();
  }

  /**
   * @brief Receive a new market data snapshot.
   *
   * Called externally (typically from the IB API market data handler)
   * when a new market snapshot is available. Thread-safe.
   *
   * @param snap Latest market data snapshot.
   */
  void onSnapshot(const MarketSnapshot& snap) override {
    std::lock_guard<std::mutex> lk(mutex_);
    latest_ = snap;
    newData_ = true;
  }

private:
  /**
   * @brief Background loop that executes strategy logic.
   *
   * Periodically checks if new market data has arrived. When a new
   * snapshot is available and the price condition is met, it sends an
   * order request (if no active order exists).
   */
  void loop() {
    while (running_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      MarketSnapshot snap;
      {
        std::lock_guard<std::mutex> lk(mutex_);
        if (!newData_) continue;
        snap = latest_;
        newData_ = false;
      }

      // === Simple Strategy Logic ===
      if (snap.last > 0 && !orderActive_) {
        LOG_INFO("[SimpleStrategy] Price > 0 detected. Sending buy order...");
        placeOrder();
      }

      // Simulate "order fill" and closing after some delay
      if (orderActive_) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(500ms);  // simulate fill delay
        closeOrder();
      }
    }
  }

  /**
   * @brief Place a simple market buy order.
   *
   * Creates an OrderRequest and pushes it to the outbound queue for
   * asynchronous execution by the OrderExecutor.
   */
  void placeOrder() {
    OrderRequest req;
    req.localId = nextOrderId_++;
    // Fill these with your IB types
    // req.contract = buildContract("GOOGL");
    // req.order = buildMarketOrder("BUY", 1);
    outQueue_->push(std::move(req));
    orderActive_ = true;
  }

  /**
   * @brief Simulate closing the active order after it’s filled.
   *
   * In a real trading system, this would be triggered by broker
   * callbacks confirming the fill.
   */
  void closeOrder() {
    LOG_INFO("[SimpleStrategy] Closing active order.");
    orderActive_ = false;
  }

private:
  std::shared_ptr<ConcurrentQueue<OrderRequest>> outQueue_;  ///< Outgoing order queue.
  std::atomic<bool> running_;                                ///< Control flag for main loop.
  std::thread worker_;                                       ///< Strategy background thread.

  std::mutex mutex_;               ///< Protects access to latest market snapshot.
  MarketSnapshot latest_;          ///< Last received market snapshot.
  bool newData_{false};            ///< Flag indicating whether new data is available.
  std::atomic<bool> orderActive_;  ///< Whether an order is currently active.
  int nextOrderId_{1};             ///< Local counter for assigning order IDs.
};

#endif  // QUANTDREAMCPP_TEST_STRATEGY_H
