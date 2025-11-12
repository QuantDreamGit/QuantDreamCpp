# PositionManager Market Data Callbacks - Quick Guide

## Overview

The `PositionManager` now provides callbacks for real-time market data events. When IB sends market data updates (bid, ask, last, mid prices, or complete snapshots), the `PositionManager` can notify your strategy immediately.

## How It Works

```
IB API → IBMarketWrapper::tickPrice() → PositionManager callbacks → Your Strategy
```

## Setup Steps

### 1. Create PositionManager Instance

```cpp
PositionManager positionManager;
```

### 2. Wire to IBStrategyWrapper

```cpp
IBStrategyWrapper ib;
IB::Helpers::ensureConnected(ib, "127.0.0.1", 4002, 0);

// Wire the position manager to receive both position AND market data updates
ib.setPositionManager(&positionManager);
```

### 3. Register Your Callbacks

Before requesting market data, register callbacks on the `PositionManager`:

```cpp
// Callback fires every time a BID price is received
positionManager.setOnBidCallback([](int tickerId, double bid) {
    std::cout << "New bid for " << tickerId << ": " << bid << std::endl;
    // Your logic here
});

// Callback fires every time an ASK price is received
positionManager.setOnAskCallback([](int tickerId, double ask) {
    std::cout << "New ask for " << tickerId << ": " << ask << std::endl;
    // Your logic here
});

// Callback fires every time a LAST trade price is received
positionManager.setOnLastCallback([](int tickerId, double last) {
    std::cout << "Last trade for " << tickerId << ": " << last << std::endl;
    // Your logic here
});

// Callback fires when both bid and ask are available (mid = (bid+ask)/2)
positionManager.setOnMidCallback([](int tickerId, double mid) {
    std::cout << "Mid price for " << tickerId << ": " << mid << std::endl;
    // Your logic here
});

// Callback fires when a complete snapshot is ready
// (all required fields based on the request mode)
positionManager.setOnSnapshotCallback([](int tickerId, 
                                          const IB::MarketData::MarketSnapshot& snapshot) {
    std::cout << "Complete snapshot for " << tickerId << std::endl;
    std::cout << "  Bid: " << snapshot.bid << std::endl;
    std::cout << "  Ask: " << snapshot.ask << std::endl;
    std::cout << "  Last: " << snapshot.last << std::endl;
    
    if (snapshot.hasGreeks) {
        std::cout << "  Delta: " << snapshot.delta << std::endl;
        std::cout << "  IV: " << snapshot.impliedVol << std::endl;
    }
    
    // Your complete snapshot logic here
});
```

### 4. Request Market Data

After setting up callbacks, request market data as usual:

```cpp
Contract stock = IB::Contracts::makeStock("AAPL", "SMART", "USD");
int tickerId = 1001;

// For streaming data (continuous updates)
ib.client->reqMktData(tickerId, stock, "", false, false, TagValueListSPtr());

// OR for a one-time snapshot
// ib.client->reqMktData(tickerId, stock, "", true, false, TagValueListSPtr());
```

Now your callbacks will fire automatically as market data arrives!

## Where to Place Callbacks in Your Strategy

### Option A: In a Strategy Class Constructor

```cpp
class MyStrategy {
public:
    MyStrategy(PositionManager& pm) : pm_(pm) {
        // Register callbacks in constructor
        pm_.setOnMidCallback([this](int tickerId, double mid) {
            this->handleMidUpdate(tickerId, mid);
        });
        
        pm_.setOnSnapshotCallback([this](int tickerId, const auto& snap) {
            this->handleSnapshot(tickerId, snap);
        });
    }
    
private:
    void handleMidUpdate(int tickerId, double mid) {
        // Your strategy logic
    }
    
    void handleSnapshot(int tickerId, const IB::MarketData::MarketSnapshot& snap) {
        // Your strategy logic
    }
    
    PositionManager& pm_;
};
```

### Option B: In a Setup Function

```cpp
void setupStrategy(PositionManager& pm) {
    pm.setOnBidCallback([&](int tickerId, double bid) {
        // Handle bid update
    });
    
    pm.setOnAskCallback([&](int tickerId, double ask) {
        // Handle ask update
    });
    
    pm.setOnMidCallback([&](int tickerId, double mid) {
        // Handle mid price
    });
}
```

### Option C: In main() Before Requesting Data

```cpp
int main() {
    IBStrategyWrapper ib;
    PositionManager pm;
    ib.setPositionManager(&pm);
    
    // Register callbacks
    pm.setOnMidCallback([](int id, double mid) {
        std::cout << "Mid: " << mid << std::endl;
    });
    
    // Now request market data
    ib.client->reqMktData(1001, myContract, "", false, false, TagValueListSPtr());
    
    // Process messages
    while (running) {
        ib.client->processMessages();
    }
}
```

## Available Callbacks

| Callback | When It Fires | Parameters |
|----------|---------------|------------|
| `setOnBidCallback` | IB sends BID price update | `(int tickerId, double bid)` |
| `setOnAskCallback` | IB sends ASK price update | `(int tickerId, double ask)` |
| `setOnLastCallback` | IB sends LAST trade price | `(int tickerId, double last)` |
| `setOnMidCallback` | Both bid and ask available | `(int tickerId, double mid)` |
| `setOnSnapshotCallback` | Complete snapshot ready | `(int tickerId, const MarketSnapshot& snap)` |

## Common Use Cases

### 1. Track Spread in Real-Time

```cpp
std::map<int, double> bids, asks;

pm.setOnBidCallback([&](int id, double bid) {
    bids[id] = bid;
    if (asks.count(id)) {
        double spread = asks[id] - bid;
        std::cout << "Spread for " << id << ": " << spread << std::endl;
    }
});

pm.setOnAskCallback([&](int id, double ask) {
    asks[id] = ask;
    if (bids.count(id)) {
        double spread = ask - bids[id];
        std::cout << "Spread for " << id << ": " << spread << std::endl;
    }
});
```

### 2. Execute Trade When Mid Price Crosses Threshold

```cpp
pm.setOnMidCallback([&](int tickerId, double mid) {
    if (mid > 150.0 && !hasPosition) {
        std::cout << "Mid price crossed 150! Placing order..." << std::endl;
        // Place your order here
    }
});
```

### 3. Delta Hedging with Greeks

```cpp
pm.setOnSnapshotCallback([&](int tickerId, const auto& snap) {
    if (snap.hasGreeksData()) {
        double totalDelta = calculateTotalDelta(positions);
        if (std::abs(totalDelta) > 0.1) {
            std::cout << "Rebalancing delta: " << totalDelta << std::endl;
            // Hedge with underlying
        }
    }
});
```

## Full Example

See `standalone/source/ibkr/position_manager_example.cpp` for a complete working example.

## Notes

- All callbacks are **optional** - only register the ones you need
- Callbacks are **thread-safe** - they may be called from IB's callback thread
- `tickerId` is the request ID you used in `reqMktData()`
- For streaming data, callbacks fire on every update
- For snapshots, the snapshot callback fires once when data is complete
- The mid callback fires whenever BOTH bid and ask are available

## Timing

- **Bid/Ask/Last callbacks**: Fire immediately when IB sends the tick
- **Mid callback**: Fires after both bid and ask are received
- **Snapshot callback**: Fires when all required fields are available (based on `PriceType` mode)

## Thread Safety

The `PositionManager` is thread-safe. Your callbacks can safely:
- Call `positionManager.snapshot()` to get current positions
- Access shared data structures (but protect them with mutexes)
- Call IB API functions (through the wrapper)
