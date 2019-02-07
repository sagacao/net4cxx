//
// Created by yuwenyong on 17-9-29.
//

#include "net4cxx/net4cxx.h"


using namespace net4cxx;


class MyProtocol: public Protocol, public std::enable_shared_from_this<MyProtocol> {
public:
    void connectionMade() override {
        NET4CXX_LOG_INFO(gAppLog, "Connection made");

    }

    void dataReceived(Byte *data, size_t length) override {
        std::string s((char *)data, (char *)data + length);
        NET4CXX_LOG_INFO(gAppLog, "Data received: %s", s.c_str());
        write(data, length);
    }

    void connectionLost(std::exception_ptr reason) override {
        NET4CXX_LOG_INFO(gAppLog, "Connection lost");
    }
};


class MyFactory: public Factory {
public:
    ProtocolPtr buildProtocol(const Address &address) override {
        NET4CXX_LOG_INFO(gAppLog, "Build protocol");
        return std::make_shared<MyProtocol>();
    }
};


class TCPServerApp: public AppBootstrapper {
public:
    using AppBootstrapper::AppBootstrapper;

    void onRun() override {
//        TCPServerEndpoint endpoint(reactor(), "28001");
//        endpoint.listen(std::make_shared<MyFactory>());
        serverFromString(reactor(), "tcp:28001")->listen(std::make_shared<MyFactory>())->addCallback([](DeferredValue val) {
            auto listener = val.asShared<Listener>();
            NET4CXX_LOG_ERROR(gAppLog, "Listen started");
            return val;
//            return listener->stopListening()->addCallback([](DeferredValue val) {
//                NET4CXX_LOG_ERROR(gAppLog, "Listen stopped");
//                return nullptr;
//            });
        });
//        serverFromString(reactor(), "ssl:28001:privateKey=test.key:certKey=test.crt")->listen(std::make_shared<MyFactory>());
//        serverFromString(reactor(), "unix:/data/foo/bar")->listen(std::make_shared<MyFactory>());
//        reactor()->resolve("localhost", [](StringVector addresses) {
//            NET4CXX_LOG_INFO(gAppLog, "resolve localhost");
//            for (auto &addr: addresses) {
//                NET4CXX_LOG_INFO("address %s", addr.c_str());
//            }
//        });
//        reactor()->resolve("unknlocalhost", [](StringVector addresses) {
//            NET4CXX_LOG_INFO(gAppLog, "resolve unknlocalhost");
//            for (auto &addr: addresses) {
//                NET4CXX_LOG_INFO("address %s", addr.c_str());
//            }
//        });
    }
};

int main(int argc, char **argv) {
    TCPServerApp app;
    app.run(argc, argv);
    return 0;
}