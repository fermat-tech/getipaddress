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
    std::cerr << "Testing" << '\n';
    do_work(argc_ - 1, argv_ + 1);
    std::exit(exit_code);
}

namespace // Anonymous namespace
{

bool do_work(int argc_, char **argv_)
{
    auto success = true;
    for (auto i = 0; i < argc_; ++i)
    {
        std::string hostname = *(argv_ + i);
        std::cout << hostname << '\n';
    }
    return success;
}

}
