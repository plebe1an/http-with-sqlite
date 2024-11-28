#include "./include/Service.h"


int main()
{
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    std::cout << "START" << std::endl;

    try
    {
        auto const address = net::ip::make_address("0.0.0.0");
        unsigned short port = 8080;

        net::io_context ioc{ 1 };

        auto listener = std::make_shared<Listener>(ioc, tcp::endpoint{ address, port });

        ioc.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}