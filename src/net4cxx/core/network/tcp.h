//
// Created by yuwenyong on 17-9-26.
//

#ifndef NET4CXX_CORE_NETWORK_TCP_H
#define NET4CXX_CORE_NETWORK_TCP_H

#include "net4cxx/common/common.h"
#include <boost/asio.hpp>
#include "net4cxx/common/debugging/watcher.h"
#include "net4cxx/common/global/loggers.h"
#include "net4cxx/core/network/base.h"

NS_BEGIN

class Factory;
class ClientFactory;
class TCPConnector;

class NET4CXX_COMMON_API TCPConnection: public Connection, public std::enable_shared_from_this<TCPConnection> {
public:
    using SocketType = boost::asio::ip::tcp::socket;

    TCPConnection(const ProtocolPtr &protocol, Reactor *reactor);

    SocketType& getSocket() {
        return _socket;
    }

    void write(const Byte *data, size_t length) override;

    void loseConnection() override;

    void abortConnection() override;

    bool getNoDelay() const override {
        boost::asio::ip::tcp::no_delay option;
        _socket.get_option(option);
        return option.value();
    }

    void setNoDelay(bool enabled) override {
        boost::asio::ip::tcp::no_delay option(enabled);
        _socket.set_option(option);
    }

    bool getKeepAlive() const override {
        boost::asio::socket_base::keep_alive option;
        _socket.get_option(option);
        return option.value();
    }

    void setKeepAlive(bool enabled) override {
        boost::asio::socket_base::keep_alive option(enabled);
        _socket.set_option(option);
    }

    std::string getLocalAddress() const override {
        auto endpoint = _socket.local_endpoint();
        return endpoint.address().to_string();
    }

    unsigned short getLocalPort() const override {
        auto endpoint = _socket.local_endpoint();
        return endpoint.port();
    }

    std::string getRemoteAddress() const override {
        auto endpoint = _socket.remote_endpoint();
        return endpoint.address().to_string();
    }

    unsigned short getRemotePort() const override {
        auto endpoint = _socket.remote_endpoint();
        return endpoint.port();
    }
protected:
    void doClose();

    void doAbort();

    virtual void closeSocket();

    void startReading() {
        if (!_reading) {
            doRead();
        }
    }

    void doRead();

    void cbRead(const boost::system::error_code &ec, size_t transferredBytes) {
        _reading = false;
        handleRead(ec, transferredBytes);
        if (!_disconnecting && !_disconnected) {
            doRead();
        }
    }

    void handleRead(const boost::system::error_code &ec, size_t transferredBytes);

    void startWriting() {
        if (!_writing) {
            doWrite();
        }
    }

    void doWrite();

    void cbWrite(const boost::system::error_code &ec, size_t transferredBytes) {
        _writing = false;
        handleWrite(ec, transferredBytes);
        if (!_aborting && !_disconnected && !_writeQueue.empty()) {
            doWrite();
        }
    }

    void handleWrite(const boost::system::error_code &ec, size_t transferredBytes);

    SocketType _socket;
    std::exception_ptr _error;
};


class NET4CXX_COMMON_API TCPServerConnection: public TCPConnection {
public:
    explicit TCPServerConnection(Reactor *reactor)
            : TCPConnection({}, reactor) {
#ifdef NET4CXX_DEBUG
        NET4CXX_Watcher->inc(NET4CXX_TCPServerConnection_COUNT);
#endif
    }

#ifdef NET4CXX_DEBUG
    ~TCPServerConnection() override {
        NET4CXX_Watcher->dec(NET4CXX_TCPServerConnection_COUNT);
    }
#endif

    void cbAccept(const ProtocolPtr &protocol);
};


class NET4CXX_COMMON_API TCPClientConnection: public TCPConnection {
public:
    explicit TCPClientConnection(Reactor *reactor)
            : TCPConnection({}, reactor) {
#ifdef NET4CXX_DEBUG
        NET4CXX_Watcher->inc(NET4CXX_TCPClientConnection_COUNT);
#endif
    }

#ifdef NET4CXX_DEBUG
    ~TCPClientConnection() override {
        NET4CXX_Watcher->dec(NET4CXX_TCPClientConnection_COUNT);
    }
#endif

    void cbConnect(const ProtocolPtr &protocol, std::shared_ptr<TCPConnector> connector);
protected:
    void closeSocket() override;

    std::shared_ptr<TCPConnector> _connector;
};


class NET4CXX_COMMON_API TCPListener: public Listener, public std::enable_shared_from_this<TCPListener> {
public:
    using AddressType = boost::asio::ip::address;
    using AcceptorType = boost::asio::ip::tcp::acceptor;
    using SocketType = boost::asio::ip::tcp::socket;
    using ResolverType = boost::asio::ip::tcp::resolver;
    using EndpointType = boost::asio::ip::tcp::endpoint;
    using ResolverIterator = ResolverType::iterator;

    TCPListener(std::string port, std::shared_ptr<Factory> factory, std::string interface, Reactor *reactor);

#ifdef NET4CXX_DEBUG
    ~TCPListener() override {
        NET4CXX_Watcher->dec(NET4CXX_TCPListener_COUNT);
    }
#endif

    void startListening() override;

    void stopListening() override;

    std::string getLocalAddress() const {
        auto endpoint = _acceptor.local_endpoint();
        return endpoint.address().to_string();
    }

    unsigned short getLocalPort() const {
        auto endpoint = _acceptor.local_endpoint();
        return endpoint.port();
    }
protected:
    void cbAccept(const boost::system::error_code &ec);

    void handleAccept(const boost::system::error_code &ec);

    void doAccept() {
        _connection = std::make_shared<TCPServerConnection>(_reactor);
        _acceptor.async_accept(_connection->getSocket(), std::bind(&TCPListener::cbAccept, shared_from_this(),
                                                                   std::placeholders::_1));
    }

    std::string _port;
    std::shared_ptr<Factory> _factory;
    std::string _interface;
    AcceptorType _acceptor;
    bool _connected{false};
    std::shared_ptr<TCPServerConnection> _connection;
};


class NET4CXX_COMMON_API TCPConnector: public Connector, public std::enable_shared_from_this<TCPConnector> {
public:
    using AddressType = boost::asio::ip::address;
    using SocketType = boost::asio::ip::tcp::socket;
    using ResolverType = boost::asio::ip::tcp::resolver;
    using ResolverResultsType = ResolverType::results_type;
    using EndpointType = boost::asio::ip::tcp::endpoint;

    TCPConnector(std::string host, std::string port, std::shared_ptr<ClientFactory> factory, double timeout,
                 Address bindAddress, Reactor *reactor);

#ifdef NET4CXX_DEBUG
    ~TCPConnector() override {
        NET4CXX_Watcher->dec(NET4CXX_TCPConnector_COUNT);
    }
#endif

    void startConnecting() override;

    void stopConnecting() override;

    void connectionFailed(std::exception_ptr reason={});

    void connectionLost(std::exception_ptr reason={});
protected:
    ProtocolPtr buildProtocol(const Address &address);

    void cancelTimeout() {
        if (!_timeoutId.cancelled()) {
            _timeoutId.cancel();
        }
    }

    void doResolve();

    void cbResolve(const boost::system::error_code &ec, const ResolverResultsType &results) {
        handleResolve(ec, results);
        if (_state == kConnecting) {
            doConnect(results);
        }
    }

    void handleResolve(const boost::system::error_code &ec, const ResolverResultsType &results);

    void doConnect();

    void doConnect(const ResolverResultsType &results);

    void cbConnect(const boost::system::error_code &ec) {
        handleConnect(ec);
    }

    void handleConnect(const boost::system::error_code &ec);

    void cbTimeout() {
        handleTimeout();
    }

    void handleTimeout();

    void makeTransport();

    enum State {
        kDisconnected,
        kConnecting,
        kConnected,
    };

    std::string _host;
    std::string _port;
    std::shared_ptr<ClientFactory> _factory;
    double _timeout{0.0};
    Address _bindAddress;
    ResolverType _resolver;
    std::shared_ptr<TCPClientConnection> _connection;
    State _state{kDisconnected};
    DelayedCall _timeoutId;
    bool _factoryStarted{false};
    std::exception_ptr _error;
};


NS_END

#endif //NET4CXX_CORE_NETWORK_TCP_H
