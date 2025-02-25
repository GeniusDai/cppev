#include <filesystem>
#include "cppev/logger.h"

int main(int argc, char **argv) {
    std::string log_file_path = std::string(std::filesystem::path(argv[0]).parent_path())
        + "/logger_output_file.log";

    // Configure logger
    std::ofstream log_file(log_file_path);
    cppev::logger::get_instance().add_output_stream(log_file);
    cppev::logger::get_instance().set_log_level(cppev::log_level::debug);

    // Stream-style logging
    LOG_DEBUG << "LOG_DEBUG Message";
    LOG_INFO << "LOG_INFO Message";
    LOG_WARNING << "LOG_WARNING Message";
    LOG_ERROR << "LOG_ERROR Message";

    auto logging_task = []() {
        // Formatted logging
        LOG_DEBUG_FMT("LOG_DEBUG_FMT Message : %s %d", "count", 1);
        LOG_INFO_FMT("LOG_INFO_FMT Message : %s %d", "count", 2);
        LOG_WARNING_FMT("LOG_WARNING_FMT Message : %s %d", "count", 3);
        LOG_ERROR_FMT("LOG_ERROR_FMT Message : %s %d", "count", 4);
    };

    std::thread thr(logging_task);

    thr.join();

    return 0;
}
