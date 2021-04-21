#include "acmacs-base/argv.hh"
#include "acmacs-base/read-file.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    argument<str> source{*this, arg_name{"source.tsv"}, mandatory};
};

int main(int argc, const char* argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const std::string src{acmacs::file::read(opt.source)};

        // fmt::print("{}\n", titers);
    }
    catch (std::exception& err) {
        AD_ERROR("{}", err);
        exit_code = 1;
    }
    return exit_code;
}

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
