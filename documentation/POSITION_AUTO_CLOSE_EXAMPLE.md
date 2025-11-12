# Position Auto-Close Example

## Overview

The `position_manager_example.cpp` demonstrates how to automatically close positions when they are detected using the PositionManager callback system.

## What Was Implemented

### 1. Added Position Callback to PositionManager

**File**: `external/IBWrapper/include/strategy/position_manager.h`

Added a new callback hook that fires whenever a position is received from the IB API:

```cpp
// Register callback for position updates
positionManager.setOnPositionCallback([](const IB::Accounts::PositionInfo& position) {
    // This fires every time IB sends a position update
    std::cout << "Position detected: " << position.contract.symbol << std::endl;
});
```

**Key Features:**
- Thread-safe callback invocation
- Called immediately when `onPosition()` receives data from IB
- Passes complete `PositionInfo` structure with contract details, quantity, and avg cost

### 2. Example Strategy with Auto-Close Logic

**File**: `standalone/source/ibkr/position_manager_example.cpp`

The example demonstrates:

#### Registering the Position Callback

```cpp
positionManager_.setOnPositionCallback([this](const IB::Accounts::PositionInfo& position) {
    std::cout << "[Strategy] Position detected: " << position.contract.symbol 
              << " Qty: " << position.position << std::endl;
    
    // Custom logic: automatically close this position
    onPositionDetected(position);
});
```

#### Automatically Closing Positions

```cpp
void onPositionDetected(const IB::Accounts::PositionInfo& position) {
    // Create an opposing market order
    Order closeOrder;
    closeOrder.action = (position.position > 0) ? "SELL" : "BUY";  // Opposite direction
    closeOrder.orderType = "MKT";
    closeOrder.totalQuantity = static_cast<Decimal>(std::abs(position.position));
    closeOrder.transmit = true;
    
    // Place the order
    int orderId = ibWrapper_.nextOrderId();
    ibWrapper_.client->placeOrder(orderId, position.contract, closeOrder);
    
    std::cout << "[Strategy] Placed closing order #" << orderId << std::endl;
}
```

## How It Works

1. **Connection**: Connect to IB Gateway/TWS
2. **Wire PositionManager**: `ib.setPositionManager(&positionManager)`
3. **Register Callback**: In your strategy's `setupCallbacks()`, register `setOnPositionCallback()`
4. **Request Positions**: Call `ib.client->reqPositions()`
5. **Auto-Close**: When IB sends position data:
   - `IBAccountWrapper::position()` is called
   - It calls `positionManager.onPosition()`
   - Which triggers your callback
   - Your callback calls `onPositionDetected()`
   - Which places an opposing market order to close the position

## Call Flow

```
IB Gateway → IBAccountWrapper::position()
           → PositionManager::onPosition()
           → Your registered callback
           → onPositionDetected()
           → ibWrapper_.client->placeOrder() (close order)
```

## Usage Example

```cpp
int main() {
    IBStrategyWrapper ib;
    IB::Helpers::ensureConnected(ib, "127.0.0.1", 4002, 0);
    
    PositionManager positionManager;
    ib.setPositionManager(&positionManager);
    
    ExampleStrategy strategy(positionManager, ib);
    
    // Request positions - this will trigger auto-close for any open positions
    ib.client->reqPositions();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Continue with your strategy...
}
```

## Key Points

- **Custom Implementation**: This auto-close logic is in the example file only, not forced in the wrapper
- **Flexible**: You can modify `onPositionDetected()` to implement any custom logic:
  - Close only specific symbols
  - Close only if position exceeds a threshold
  - Hedge instead of close
  - Log and alert instead of closing
- **Thread-Safe**: The callback is thread-safe and can be called from IB's callback thread
- **Real-Time**: Triggers immediately when positions are received from IB

## Available Position Callbacks

| Method | When Called | Use Case |
|--------|-------------|----------|
| `setOnPositionCallback()` | Each position received | Auto-close, alerts, hedging |
| `onPositionClear()` | After all positions sent | Snapshot complete marker |
| `snapshot()` | On-demand | Get current positions anytime |

## Customization Examples

### Close Only Stocks

```cpp
void onPositionDetected(const IB::Accounts::PositionInfo& position) {
    if (position.contract.secType == "STK") {
        // Close logic here
    }
}
```

### Close Only Large Positions

```cpp
void onPositionDetected(const IB::Accounts::PositionInfo& position) {
    if (std::abs(position.position) > 100) {
        // Close logic here
    }
}
```

### Hedge Instead of Close

```cpp
void onPositionDetected(const IB::Accounts::PositionInfo& position) {
    // Buy protective options instead of closing
    if (position.contract.secType == "STK" && position.position > 0) {
        buyProtectivePut(position);
    }
}
```

## Files Modified

1. `external/IBWrapper/include/strategy/position_manager.h`
   - Added `setOnPositionCallback()` method
   - Added `onPositionCallback_` member variable
   - Modified `onPosition()` to invoke callback

2. `standalone/source/ibkr/position_manager_example.cpp`
   - Added `IBStrategyWrapper&` reference to `ExampleStrategy`
   - Added `setOnPositionCallback()` in `setupCallbacks()`
   - Implemented `onPositionDetected()` with auto-close logic
   - Updated main() to request positions

## Building

```bash
cd /home/user/CLionProjects/QuantDreamCpp
cmake --build cmake-build-debug --target IBKR_position_manager_example
```

## Running

```bash
# Make sure IB Gateway or TWS is running on localhost:4002
./cmake-build-debug/IBKR_position_manager_example
```

The example will:
1. Connect to IB Gateway
2. Request current positions
3. Automatically close any detected positions
4. Display all actions in the console

## Notes

- Make sure IB Gateway/TWS is running before executing
- The example uses market orders - be careful in live trading
- Position callbacks fire for each position, so multiple positions = multiple callbacks
- The callback is called from IB's thread - keep processing quick or dispatch to another thread
