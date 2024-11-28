#include "../include/Service.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <SQLiteCpp/SQLiteCpp.h>

// realtime for logs
std::string get_current_time()
{
    auto now = boost::posix_time::second_clock::local_time();
    std::ostringstream oss;
    oss << now;
    return oss.str();
}

// This function produces an HTTP response for the given request.
http::response<http::string_body> handle_request(http::request<http::string_body> const& req)
{

    SQLite::Database db("database.db");
    json::object json_response;


    //GET
    if (req.method() == http::verb::get)
    {

        std::string target = req.target();
        if (target.compare(0, 5, "/api/") != 0) 
        {
            return http::response<http::string_body>{http::status::not_found, req.version()};
        }

        std::string table_name = target.substr(5); //Extract the table name after "/api/"

        if (table_name.empty() || table_name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_") != std::string::npos) 
        {
            json_response["error"] = "Invalid table name";
            return http::response<http::string_body>{http::status::bad_request, req.version()};
        }

        const std::string queryStr = "SELECT * FROM " + table_name;

        SQLite::Statement   query(db, queryStr.c_str());
        json::array users;

        while (query.executeStep())
        {
            json::object user;

            for (int i = 0; i < query.getColumnCount(); i++)
            {
                const char* col = query.getColumn(i);
                user[query.getColumnName(i)] = col ? std::string(col) : "";
            }
            users.push_back(user);
        }

        json_response[table_name] = users;

        http::response<http::string_body> res{ http::status::ok, req.version() };
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = json::serialize(json_response);
        res.prepare_payload();
        return res;
    }

    /*
    // POST
    else if (req.method() == http::verb::post && req.target() == "/api/data") 
    {
        json::value json_request = json::parse(req.body());
        std::string response_message = "Received: " + json::serialize(json_request);

        json::object json_response;
        json_response["message"] = response_message;

        http::response<http::string_body> res{ http::status::ok, req.version() };
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());

        res.body() = json::serialize(json_response);
        res.prepare_payload();
        return res;
    }
    //PUT
    else if (req.method() == http::verb::put && req.target() == "/api/data")
    {
        auto json_request = boost::json::parse(req.body());
        std::string response_message = "Updated: " + boost::json::serialize(json_request);

        json::object json_response;
        json_response["message"] = response_message;

        http::response<http::string_body> res{ http::status::ok, req.version() };
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = boost::json::serialize(json_response);
        res.prepare_payload();

        return res;
    }
    //DELETE
    else if (req.method() == http::verb::delete_ && req.target() == "/api/data")
    {
        boost::json::object json_response;
        json_response["message"] = "Resource deleted";

        http::response<http::string_body> res{ http::status::ok, req.version() };
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());

        res.body() = boost::json::serialize(json_response);
        res.prepare_payload();

        return res;
    }
    */
    // Default response for unsupported methods
    return http::response<http::string_body>{http::status::bad_request, req.version()};
}

Session::Session(tcp::socket socket) : socket_(std::move(socket)) {}

void Session::run() 
{
    do_read();
}

void Session::do_read() {
    auto self(shared_from_this());
    http::async_read(socket_, buffer_, req_, [this, self](beast::error_code ec, std::size_t) 
    {
        if (!ec) 
        {
            std::string time = get_current_time();
            std::string client_ip = socket_.remote_endpoint().address().to_string();
            unsigned short client_port = socket_.remote_endpoint().port();
            std::string method = req_.method_string();

            std::cout << "[" << time << "] "
                << "Received " << method << " request from: "
                << client_ip << ":" << client_port << std::endl;

            do_write(handle_request(req_));
        }
        else 
        {
            std::cerr << "Read error: " << ec.message() << std::endl;
        }
    });
}

void Session::do_write(http::response<http::string_body> res) 
{
    auto self(shared_from_this());
    auto sp = std::make_shared<http::response<http::string_body>>(std::move(res));

    http::async_write(socket_, *sp, [this, self, sp](beast::error_code ec, std::size_t) 
    {
        socket_.shutdown(tcp::socket::shutdown_send, ec);
    });
}

Listener::Listener(net::io_context& ioc, tcp::endpoint endpoint): ioc_(ioc), acceptor_(net::make_strand(ioc)) 
{
    beast::error_code ec;

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) 
    {
        std::cerr << "Open error: " << ec.message() << std::endl;
        return;
    }

    // Allow address reuse
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) 
    {
        std::cerr << "Set option error: " << ec.message() << std::endl;
        return;
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if (ec) 
    {
        std::cerr << "Bind error: " << ec.message() << std::endl;
        return;
    }

    // Start listening for connections
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) 
    {
        std::cerr << "Listen error: " << ec.message() << std::endl;
        return;
    }

    do_accept();
}

void Listener::do_accept() 
{
    acceptor_.async_accept(net::make_strand(ioc_), [this](beast::error_code ec, tcp::socket socket) 
    {
        if (!ec) 
        {
            std::make_shared<Session>(std::move(socket))->run();
        }
        else 
        {
            std::cerr << "Accept error: " << ec.message() << std::endl;
        }
        do_accept();
    });
}

