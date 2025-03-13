#include <filesystem>

#include "cppev/logger.h"

int main(int argc, char **argv)
{
    std::string log_file_path =
        std::string(std::filesystem::path(argv[0]).parent_path()) +
        "/logger_output_file.log";

    // Configure logger
    std::ofstream log_file(log_file_path);
    cppev::logger::get_instance().add_output_stream(log_file);

    std::vector<cppev::log_level> levels = {
        cppev::log_level::debug,   cppev::log_level::info,
        cppev::log_level::warning, cppev::log_level::error,
        cppev::log_level::fatal,
    };

    for (auto level : levels)
    {
        cppev::logger::get_instance().set_log_level(level);

        // Stream-style logging
        LOG_DEBUG << "LOG_DEBUG Message" << " " << std::string("count") << " "
                  << 1;
        LOG_INFO << "LOG_INFO Message" << " " << std::string("count") << " "
                 << 2;
        LOG_WARNING << "LOG_WARNING Message" << " " << std::string("count")
                    << " " << 3;
        LOG_ERROR << "LOG_ERROR Message" << " " << std::string("count") << " "
                  << 4;
        LOG_FATAL << "LOG_FATAL Message" << " " << std::string("count") << " "
                  << 5;

        auto logging_task = []()
        {
            // Formatted logging
            LOG_DEBUG_FMT("LOG_DEBUG_FMT Message : %s %d", "count", 1);
            LOG_INFO_FMT("LOG_INFO_FMT Message : %s %d", "count", 2);
            LOG_WARNING_FMT("LOG_WARNING_FMT Message : %s %d", "count", 3);
            LOG_ERROR_FMT("LOG_ERROR_FMT Message : %s %d", "count", 4);
            LOG_FATAL_FMT("LOG_FATAL_FMT Message : %s %d", "count", 5);
        };
        std::thread thr(logging_task);
        thr.join();

        std::cout << std::endl;
    }

    return 0;
}
