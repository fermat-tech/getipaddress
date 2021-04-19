#include <boost/asio.hpp>
#include <iostream>
#include <cstdlib>
#include <string>
#include <filesystem>
#include <vector>
#include <mutex>
#include <thread>
#include <memory>

namespace // Anonymous namespace
{

// Command line options.
struct Options
{
    using host_list_t = std::vector<std::string>;
    bool ipv4 = false;
    bool ipv6 = false;
    int  nthreads = 0;
    host_list_t host_list;
};

auto program_name = std::string{};
bool parse_cmd_line(int argc_, char **argv_, Options& opts_);
bool do_work(const Options& opts_);
bool async_do_work(const Options& opts_);
void usage(const std::string& pname_);

} // Anonymous namespace

int
main(int argc_, char **argv_)
{
    auto exit_code = 0; // Program exit code.
    // Get the program name.
    {
        using std::filesystem::path;
        program_name = path{ *(argv_) }.stem().generic_string();
    }
    Options opts; // The command line options.
    // Parse the command line.
    auto parse_result = parse_cmd_line(argc_ - 1, argv_ + 1, opts);
    if (!parse_result) // If command line parsing failed.
    {
        usage(program_name); // Print help information.
        std::exit(1); // Exit the program.
    }
    auto success = false; // Indicator for errors resolving hostnames.
    if (!opts.nthreads) // If not asynchronous
    {
        success = do_work(opts); // Resolve hostnames synchronously.
    }
    else // Asynchronous resolution requested
    {
        success = async_do_work(opts); // Resolve hostnames asynchronously.
    }
    if (!success) // If any hostname wasn't resolved.
    {
        exit_code = 2; // Exit code indicating some hostnames could not be resolved.
        std::cerr << '\n'
            << program_name 
            << ": NOTICE: Some hostnames could not be resolved." 
            << '\n';
    }
    std::exit(exit_code); // Exit the program.
}

namespace // Anonymous namespace
{

// Parse the command line. Returns true for success, failure for errors or help.
bool parse_cmd_line(int argc_, char **argv_, Options& opts_)
{
    auto errors = false;
    auto accumulate = false;
    for (auto i = 0; i < argc_; ++i)
    {
        std::string opt = *(argv_ + i); // Option or hostname.
        if (accumulate) // Everything is a hostname now.
            opts_.host_list.push_back(opt);
        // Print IPv4 address version results.
        else if (opt == "--4" || opt == "-4" || opt == "--ipv4" || opt == "-ipv4")
            opts_.ipv4 = true;
        // Print IPv6 address version results.
        else if (opt == "--6" || opt == "-6" || opt == "--ipv6" || opt == "-ipv6")
            opts_.ipv6 = true;
        // Mark all remaining arguments as hostnames.
        else if (opt == "--" || opt == "-")
            accumulate = true;
        // Do hostname resolution and printing asynchronously.
        else if (opt == "--threads" || opt == "-threads" || opt == "-t")
        {
            if (++i < argc_)
            {
                std::string int_str = *(argv_ + i);
                try
                {
                    auto pos = std::string::size_type{};
                    opts_.nthreads = std::abs(std::stoi(int_str, &pos));
                    if (pos != int_str.size())
                        throw std::invalid_argument("Not an integer");
                }
                catch (const std::exception& e_)
                {
                    errors = true;
                    std::cerr << "ERROR: Argument to option " << opt 
                        << " is not an integer: " << int_str << '\n';
                }
            }
            else {
                errors = true;
                std::cerr << "ERROR: Missing argument to option " << opt << '\n';
            }
        }
        // Request help/usage information.
        else if (opt == "--help" || opt == "-help" || opt == "-h")
            errors = true; // Cause help message.
        // If not an option from above, it's a hostname,
        // and so are the rest of the arguments.
        else {
            accumulate = true; // The rest of the arguments are hostnames.
            --i; // Backtrack, so this hostname is added it to the list on the next iteration.
        }
    }
    // Missing hostname arguments.
    if (!opts_.host_list.size())
        errors = true;
    // If no parsing errors, default to operating on both ipv4 and ipv6 results if not specified.
    if (!errors && !(opts_.ipv4 || opts_.ipv6))
        opts_.ipv4 = opts_.ipv6 = true;
    return !errors; // Return false if errors encountered, true otherwise.
}

// Define some aliases for namespaces and types.
namespace net = boost::asio;
namespace ip = net::ip;
using tcp = ip::tcp;
using error_code_t = boost::system::error_code;

// Synchronously resolve hostnames and print results.
// Returns true if all hostnames were resolved, false otherwise.
bool do_work(const Options& opts_)
{
    auto success = true;
    auto ioc = net::io_context{};
    auto ec = error_code_t{};
    auto resolver = tcp::resolver{ ioc };
    for (const auto& hostname : opts_.host_list)
    {
        auto results = resolver.resolve(hostname, "", ec); // Query results.
        // If errors were encountered
        if (ec)
        {
            success = false;
            std::cerr << "ERROR: " << hostname << ": " << ec.message() << '\n';
        }
        // No errors, so print results
        else
        {
            for (const tcp::endpoint& e : results)
            {
                // Print if IPv4 results requested or defaulted.
                if (opts_.ipv4 && e.address().is_v4())
                    std::cout << hostname << "\t" << e.address() << '\n';
                // Print if IPv6 results requested or defaulted.
                else if (opts_.ipv6 && e.address().is_v6())
                    std::cout << hostname << "\t" << e.address() << '\n';
            }
        }
    }
    return success; // Return true if no errors encountered, false otherwise.
}

// Asynchronously resolve hostnames and print results.
// Returns true if all hostnames were resolved, false otherwise.
bool async_do_work(const Options& opts_)
{
    auto success = true;
    std::vector<std::shared_ptr<net::io_context>> ioc_list;
    std::vector<tcp::resolver> res_list;
    for (auto i = 0; i < opts_.nthreads; ++i)
        ioc_list.emplace_back(std::make_shared<net::io_context>());
    for (auto i = 0; i < opts_.nthreads; ++i)
        res_list.emplace_back(*ioc_list[i]);
    static std::mutex print_mtx; // Mutex for locking std::cout/cerr resources.
    auto i = 0;
    for (const auto& hostname : opts_.host_list)
    {
        auto process_results = [hostname, &opts_, &success]
            (const error_code_t& ec_, tcp::resolver::results_type results_)
        {
            std::unique_lock<std::mutex> lck{ print_mtx };
            if (ec_)
            {
                success = false;
                std::cerr << "ERROR: " << hostname 
                    << ": " << ec_.message() << '\n';
            }
            else
            {
                for (const tcp::endpoint& ep : results_)
                {
                    if (opts_.ipv4 && ep.address().is_v4())
                        std::cout << hostname << 
                            "\t" << ep.address() << '\n';
                    else if (opts_.ipv6 && ep.address().is_v6())
                        std::cout << hostname 
                            << "\t" << ep.address() << '\n';
                }
            }
        };
        if (i >= opts_.nthreads)
            i = 0;
        res_list[i++].async_resolve(hostname, "", process_results);
    }
    std::vector<std::thread> threads;
    threads.reserve(opts_.nthreads);
    for (auto i = 0; i < opts_.nthreads; ++i)
    {
        auto ioc = ioc_list[i];
            threads.emplace_back(
                [ioc]{
                    ioc->run(); // Blocking on async work.
                }
            );
    }
    // Make sure threads are done.
    for (auto& t : threads)
        if (t.joinable()) // If the thread is joinable,
            t.join();     // join the thread to wait for its completion.
    return success; // Return true if no errors encountered, false otherwise.
}

// Print help/usage information.
void usage(const std::string& pname_)
{
    std::cerr << '\n'
        << "usage: " << pname_ << " [--help] [--ipv4|--4] [--ipv6|--6] [--threads NUM_THREADS] [--] HOSTNAME..." << '\n'
        << '\n'
        << "NOTES" << '\n'
        << '\n'
        << "The default is to print both ipv4 and ipv6 addresses." << '\n'
        << "Use --ipv4 and/or --ipv6 to print only those ip address versions." << '\n'
        << "Use -- to indicate that the following arguments are hostnames." << '\n'
        << "Use --threads NUM_THREADS to resolve hostnames simultaneously and print results as they become available." << '\n'
        << "NUM_THREADS is the number of threads to use." << '\n'
        << "Using --threads NUM_THREADS may be helpful when there are many hostnames to be resolved." << '\n'
        << "Experiment with different values to determine which works best for the machine it runs on."
        << '\n';
}

}
