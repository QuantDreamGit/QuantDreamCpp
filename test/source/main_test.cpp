#include "EWrapper.h"
#include "EClientSocket.h"
#include "EReaderOSSignal.h"
#include "EReader.h"
#include "Contract.h"
#include "wrappers/EWrapperDefault.h"

#include <iostream>
#include <memory>
#include <thread>

class IBWrapper : public EWrapperDefault {
public:
    EReaderOSSignal signal;
    std::unique_ptr<EClientSocket> client;

    IBWrapper() : signal(1000) {
        client = std::make_unique<EClientSocket>(this, &signal);
    }

    ~IBWrapper() override {
        disconnect();
    }

    void connect(const char* host = "127.0.0.1", int port = 7497, int clientId = 0) {
        if (!client->eConnect(host, port, clientId)) {
            std::cerr << "Failed to connect to IB TWS" << std::endl;
            return;
        }

        std::cout << "Connected to IB TWS" << std::endl;

        // Request delayed data mode (must come before reqMktData)
        client->reqMarketDataType(3);

        // Start background reader thread
        std::thread([this]() {
            EReader reader(client.get(), &signal);
            reader.start();
            while (client->isConnected()) {
                signal.waitForSignal();
                reader.processMsgs();
            }
        }).detach();

        // Define a stock contract
        Contract contract;
        contract.symbol   = "GOOG";
        contract.secType  = "STK";
        contract.exchange = "SMART";
        contract.currency = "USD";

        // Request delayed quote data
        client->reqMktData(1001, contract, "", false, false, TagValueListSPtr());
    }

    void disconnect() {
        if (client && client->isConnected()) {
            client->eDisconnect();
            std::cout << "Disconnected from IB TWS" << std::endl;
        }
    }

    // ----- IB API Callbacks -----

    void connectAck() override {
        std::cout << "Connection acknowledged" << std::endl;
    }

    void connectionClosed() override {
        std::cout << "Connection closed" << std::endl;
    }

    void nextValidId(OrderId orderId) override {
        std::cout << "Next valid order ID: " << orderId << std::endl;
    }

    void marketDataType(TickerId reqId, int marketDataType) override {
        std::string type;
        switch (marketDataType) {
            case 1: type = "Real-time"; break;
            case 2: type = "Frozen"; break;
            case 3: type = "Delayed"; break;
            case 4: type = "Delayed Frozen"; break;
            default: type = "Unknown"; break;
        }
        std::cout << "Market data type for request " << reqId << ": " << type << std::endl;
    }

    void tickPrice(TickerId tickerId, TickType field, double price, const TickAttrib&) override {
        if (price > 0)
            std::cout << "Tick price [" << tickerId << "] field " << field << " = " << price << std::endl;
    }

    void tickSize(TickerId tickerId, TickType field, Decimal size) override {
        std::cout << "Tick size [" << tickerId << "] field " << field
                  << " = " << static_cast<double>(size) << std::endl;
    }

    void error(int id, time_t time, int code, const std::string& msg, const std::string&) override {
        std::cerr << "Error [" << code << "] " << msg << std::endl;
    }
};

int main() {
  IBWrapper ib;
  ib.connect("127.0.0.1", 7497, 1);

  std::cout << "Press Enter to exit..." << std::endl;
  std::cin.get();

  ib.disconnect();
  return 0;
}

