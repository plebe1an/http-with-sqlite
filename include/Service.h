#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace json = boost::json;

using tcp = net::ip::tcp;

std::string get_current_time();
http::response<http::string_body> handle_request(http::request<http::string_body> const& req);

// This class handles an HTTP server connection.
class Session : public std::enable_shared_from_this<Session>
{
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;


    void do_read();
    void do_write(http::response<http::string_body> res);

public:

    explicit Session(tcp::socket socket);
    void run();
};

// This class accepts incoming connections and launches the sessions.
class Listener : public std::enable_shared_from_this<Listener>
{
    net::io_context& ioc_;
    tcp::acceptor acceptor_;

    void do_accept();

public:
    Listener(net::io_context& ioc, tcp::endpoint endpoint);
};