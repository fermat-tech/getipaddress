#include <boost/asio.hpp>
#include <iostream>
#include <cstdlib>
#include <string>
#include <filesystem>

namespace // Anonymous namespace
{

auto program_name = std::string{};
bool do_work(int argc_, char **argv_);

} // Anonymous namespace

int
main(int argc_, char **argv_)
{
    auto exit_code = 0;
    {
        using std::filesystem::path;
        program_name = path{ *(argv_) }.stem().generic_string();
    }
    if (argc_ < 2)
    {
        std::cerr << "ERROR: Missing hostname arguments." << '\n';
        std::exit(1);
    }
    auto success = do_work(argc_ - 1, argv_ + 1);
    if (!success)
    {
        exit_code = 2;
        std::cerr << '\n'
            << program_name 
            << ": NOTICE: Some hostnames could not be resolved." 
            << '\n';
    }
    std::exit(exit_code);
}

namespace // Anonymous namespace
{

namespace net = boost::asio;
namespace ip = net::ip;
using tcp = ip::tcp;
using error_code = boost::system::error_code;

bool do_work(int argc_, char **argv_)
{
    auto success = true;
    auto ioc = net::io_context{};
    auto ec = error_code{};
    auto resolver = tcp::resolver{ ioc };
    for (auto i = 0; i < argc_; ++i)
    {
        std::string hostname = *(argv_ + i);
        auto results = resolver.resolve(hostname, "80", ec);
        if (ec)
        {
            success = false;
            std::cerr << "ERROR: " << hostname << ": " << ec.message() << '\n';
        }
        else
        {
            for (const tcp::endpoint& e : results)
            {
                std::cout << hostname << "\t" << e.address() << '\n';
            }
        }
    }
    return success;
}

}
