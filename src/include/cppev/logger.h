#ifndef _cppev_logger_h_6C0224787A17_
#define _cppev_logger_h_6C0224787A17_

#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <vector>
#include <memory>
#include <chrono>
#include <iomanip>
#include <thread>
#include <cstdarg>

namespace cppev
{

// Log severity levels
enum class log_level
{
    debug,
    info,
    warning,
    error,
};

// Thread-safe logger implementation with multiple output support
class logger
{
public:
    // Singleton access point
    static logger &get_instance();

    // Set minimum log severity level
    void set_log_level(log_level level);

    // Add output stream destination
    void add_output_stream(std::ostream& output);

    // Get current log level
    log_level get_log_level() const;

    // Core logging method
    void write_log(log_level level, const std::string& file, int line, const std::string& message);

private:
    logger();

    // Convert log level to string representation
    std::string level_to_string(log_level level) const;

    // Add timestamp with millisecond precision
    void add_timestamp(std::ostream& os);

    // Add thread ID information
    void add_thread_id(std::ostream& os);

    log_level current_level_;

    std::vector<std::ostream*> output_streams_;

    std::mutex mtx_;
};

// Helper class for constructing log messages
class log_message
{
public:
    // Constructor for stream-style logging
    log_message(log_level level, const char* file, int line);

    // Constructor for printf-style formatting
    log_message(log_level level, const char* file, int line, const char* format, ...);

    ~log_message();

    // Stream interface for chaining operations
    std::ostringstream& stream();

private:
    // Safe formatted string implementation
    void format_message(const char* format, va_list args);

    log_level message_level_;

    const char* source_file_;

    int line_number_;

    std::ostringstream message_buffer_;
};

}   // namespace cppev


// Macro helpers for log interface generation
#define LOG_BASE(level) \
    if (level < cppev::logger::get_instance().get_log_level()) ; \
    else cppev::log_message(level, __FILE__, __LINE__).stream()

#define LOG_FMT_BASE(level, format, ...) \
    if (level < cppev::logger::get_instance().get_log_level()) ; \
    else cppev::log_message(level, __FILE__, __LINE__, format, ##__VA_ARGS__)


// Stream-style logging macros
#define LOG_DEBUG   LOG_BASE(cppev::log_level::debug)
#define LOG_INFO    LOG_BASE(cppev::log_level::info)
#define LOG_WARNING LOG_BASE(cppev::log_level::warning)
#define LOG_ERROR   LOG_BASE(cppev::log_level::error)

// Formatted logging macros
#define LOG_DEBUG_FMT(format, ...)   LOG_FMT_BASE(cppev::log_level::debug, format, ##__VA_ARGS__)
#define LOG_INFO_FMT(format, ...)    LOG_FMT_BASE(cppev::log_level::info, format, ##__VA_ARGS__)
#define LOG_WARNING_FMT(format, ...) LOG_FMT_BASE(cppev::log_level::warning, format, ##__VA_ARGS__)
#define LOG_ERROR_FMT(format, ...)   LOG_FMT_BASE(cppev::log_level::error, format, ##__VA_ARGS__)

#endif  // logger.h
