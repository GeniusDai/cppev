#ifndef _cppev_logger_h_6C0224787A17_
#define _cppev_logger_h_6C0224787A17_

#include <chrono>
#include <cstdarg>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

#include "cppev/common.h"

namespace cppev
{

// Define color macros.
#define RESET_COLOR "\033[0m"     // Reset to default color
#define DEBUG_COLOR "\033[34m"    // Light blue for DEBUG messages
#define INFO_COLOR "\033[32m"     // Green for INFO messages
#define WARNING_COLOR "\033[33m"  // Yellow for WARNING messages
#define ERROR_COLOR "\033[31m"    // Red for ERROR messages
#define FATAL_COLOR "\033[35m"    // Purple for FATAL messages

// Log severity levels.
enum class CPPEV_PUBLIC log_level
{
    debug = 1 << 0,
    info = 1 << 1,
    warning = 1 << 2,
    error = 1 << 3,
    fatal = 1 << 4,
};

// Thread-safe logger implementation with multiple output support.
class CPPEV_PUBLIC logger
{
public:
    // Singleton access point.
    static logger &get_instance();

    // Get current log level.
    log_level get_log_level() const;

    // Set minimum log severity level.
    void set_log_level(log_level level);

    // Add output stream destination.
    void add_output_stream(std::ostream &output);

    // Add output stream destination for specific log_level.
    void add_output_stream(log_level level, std::ostream &output);

    // Core logging method.
    void write_log(log_level level, const std::string &file, int line,
                   const std::string &message);

private:
    // Constructor which will configure output streams.
    logger();

    // Convert log level to string representation.
    std::string level_to_string(log_level level) const;

    // Add color.
    void add_color(std::ostream &os, log_level level);

    // Add timestamp with millisecond precision.
    void add_timestamp(std::ostream &os);

    // Add thread ID information.
    void add_thread_id(std::ostream &os);

    // Reset color.
    void reset_color(std::ostream &os);

    log_level current_level_;

    std::unordered_map<log_level, std::vector<std::ostream *>> output_streams_;

    std::mutex mtx_;
};

// Helper class for constructing log messages.
class CPPEV_INTERNAL log_message
{
public:
    // Constructor for stream-style logging.
    log_message(log_level level, const char *file, int line);

    // Constructor for printf-style formatting.
    log_message(log_level level, const char *file, int line, const char *format,
                ...);

    // Destructor which will call write_log.
    ~log_message();

    // Stream interface for chaining operations.
    std::ostringstream &stream();

private:
    // Safe formatted string implementation.
    void format_message(const char *format, va_list args);

    log_level message_level_;

    const char *source_file_;

    int line_number_;

    std::ostringstream message_buffer_;
};

}  // namespace cppev

// Macro helpers for log interface generation.
#define LOG_BASE(level)                                        \
    if (level < cppev::logger::get_instance().get_log_level()) \
        ;                                                      \
    else                                                       \
        cppev::log_message(level, __FILE__, __LINE__).stream()

#define LOG_FMT_BASE(level, format, ...)                       \
    if (level < cppev::logger::get_instance().get_log_level()) \
        ;                                                      \
    else                                                       \
        cppev::log_message(level, __FILE__, __LINE__, format, ##__VA_ARGS__)

// Stream-style logging macros.
#define LOG_DEBUG LOG_BASE(cppev::log_level::debug)
#define LOG_INFO LOG_BASE(cppev::log_level::info)
#define LOG_WARNING LOG_BASE(cppev::log_level::warning)
#define LOG_ERROR LOG_BASE(cppev::log_level::error)
#define LOG_FATAL LOG_BASE(cppev::log_level::fatal)

// Formatted logging macros.
#define LOG_DEBUG_FMT(format, ...) \
    LOG_FMT_BASE(cppev::log_level::debug, format, ##__VA_ARGS__)
#define LOG_INFO_FMT(format, ...) \
    LOG_FMT_BASE(cppev::log_level::info, format, ##__VA_ARGS__)
#define LOG_WARNING_FMT(format, ...) \
    LOG_FMT_BASE(cppev::log_level::warning, format, ##__VA_ARGS__)
#define LOG_ERROR_FMT(format, ...) \
    LOG_FMT_BASE(cppev::log_level::error, format, ##__VA_ARGS__)
#define LOG_FATAL_FMT(format, ...) \
    LOG_FMT_BASE(cppev::log_level::fatal, format, ##__VA_ARGS__)

#endif  // logger.h
