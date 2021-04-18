#include <boost/asio.hpp>
#include <iostream>
#include <cstdlib>
#include <string>
#include <filesystem>
#include <vector>

namespace // Anonymous namespace
{

struct Options
{
    using host_list_t = std::vector<std::string>;
    bool ipv4 = false;
    bool ipv6 = false;
    bool async = false;
    host_list_t host_list;
};

auto program_name = std::string{};
bool parse_cmd_line(int argc_, char **argv_, Options& opts_);
bool do_work(const Options& opts_);
void usage(const std::string& pname_);

} // Anonymous namespace

int
main(int argc_, char **argv_)
{
    auto exit_code = 0;
    {
        using std::filesystem::path;
        program_name = path{ *(argv_) }.stem().generic_string();
    }
    Options opts;
    auto parse_result = parse_cmd_line(argc_ - 1, argv_ + 1, opts);
    if (!parse_result)
    {
        usage(program_name);
        std::exit(1);
    }
    auto success = do_work(opts);
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

bool parse_cmd_line(int argc_, char **argv_, Options& opts_)
{
    auto errors = false;
    auto accumulate = false;
    for (auto i = 0; i < argc_; ++i)
    {
        std::string opt = *(argv_ + i);
        if (accumulate)
            opts_.host_list.push_back(opt);
        else if (opt == "--4" || opt == "-4" || opt == "--ipv4" || opt == "-ipv4")
            opts_.ipv4 = true;
        else if (opt == "--6" || opt == "-6" || opt == "--ipv6" || opt == "-ipv6")
            opts_.ipv6 = true;
        else if (opt == "--" || opt == "-")
            accumulate = true;
        else if (opt == "--async" || opt == "-async" || opt == "-a")
            opts_.async = true;
        else if (opt == "--help" || opt == "-help" || opt == "-h")
            errors = true; // Cause help message.
        else {
            accumulate = true;
            --i; // Backtrack
        }
    }
    if (!opts_.host_list.size())
        errors = true;
    if (!errors && !(opts_.ipv4 || opts_.ipv6))
        opts_.ipv4 = opts_.ipv6 = true;
    return !errors;
}

namespace net = boost::asio;
namespace ip = net::ip;
using tcp = ip::tcp;
using error_code = boost::system::error_code;

bool do_work(const Options& opts_)
{
    auto success = true;
    auto ioc = net::io_context{};
    auto ec = error_code{};
    auto resolver = tcp::resolver{ ioc };
    for (const auto& hostname : opts_.host_list)
    {
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
                if (opts_.ipv4 && e.address().is_v4())
                    std::cout << hostname << "\t" << e.address() << '\n';
                else if (opts_.ipv6 && e.address().is_v6())
                    std::cout << hostname << "\t" << e.address() << '\n';
            }
        }
    }
    return success;
}

void usage(const std::string& pname_)
{
    std::cerr << '\n'
        << "usage: " << pname_ << " [--help] [--ipv4|--4] [--ipv6|--6] [--async] [--] HOSTNAME..." << '\n'
        << '\n'
        << "NOTES" << '\n'
        << '\n'
        << "The default is to print both ipv4 and ipv6 addresses." << '\n'
        << "Use --ipv4 and/or --ipv6 to print only those ip address versions." << '\n'
        << "Use -- to indicate that the following arguments are hostnames." << '\n'
        << "Use --async to resolve hostnames simultaneously and print results as they become available." << '\n'
        << "Using --async is helpful when there are many hostnames to be resolved." << '\n'
        << '\n';
}

}
