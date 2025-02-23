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
enum class log_level {
    debug,
    info,
    warning,
    error
};

/// Thread-safe logger implementation with multiple output support
class logger {
public:
    /// Singleton access point
    static logger& get_instance() {
        static logger instance;
        return instance;
    }

    /// Set minimum log severity level
    void set_log_level(log_level level) {
        std::lock_guard<std::mutex> lock(mtx);
        current_level = level;
    }

    /// Add output stream destination
    void add_output_stream(std::ostream& output) {
        std::lock_guard<std::mutex> lock(mtx);
        output_streams.push_back(&output);
    }

    /// Get current log level
    log_level get_log_level() const { return current_level; }

    /// Core logging method
    void write_log(log_level level, const std::string& file,
                int line, const std::string& message) {
        if (level < current_level) return;

        std::stringstream log_entry;
        add_timestamp(log_entry);
        add_thread_id(log_entry);
        log_entry << " [" << level_to_string(level) << "] ";
        log_entry << "[" << file << ":" << line << "] ";
        log_entry << message << std::endl;

        std::lock_guard<std::mutex> lock(mtx);
        for (auto& stream : output_streams) {
            if (stream) {
                *stream << log_entry.str();
                stream->flush();
            }
        }
    }

private:
    logger() : current_level(log_level::info) {
        output_streams.push_back(&std::cout);
    }

    // Convert log level to string representation
    std::string level_to_string(log_level level) const {
        switch (level) {
            case log_level::debug:   return "DEBUG";
            case log_level::info:    return "INFO";
            case log_level::warning: return "WARNING";
            case log_level::error:   return "ERROR";
            default:                 return "UNKNOWN";
        }
    }

    // Add timestamp with millisecond precision
    void add_timestamp(std::ostream& os) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        os << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
           << '.' << std::setfill('0') << std::setw(3) << ms.count();
    }

    // Add thread ID information
    void add_thread_id(std::ostream& os) {
        os << " [Thread:";
#ifdef __linux__
        os << "0x";
#endif
        os << std::hex << std::this_thread::get_id() << std::dec << "]";
    }

    log_level current_level;
    std::vector<std::ostream*> output_streams;
    mutable std::mutex mtx;
};

/// Helper class for constructing log messages
class log_message {
public:
    // Constructor for stream-style logging
    log_message(log_level level, const char* file, int line)
        : message_level(level), source_file(file), line_number(line) {}

    // Constructor for printf-style formatting
    log_message(log_level level, const char* file, int line,
             const char* format, ...)
        : message_level(level), source_file(file), line_number(line) {
        va_list args;
        va_start(args, format);
        format_message(format, args);
        va_end(args);
    }

    ~log_message() {
        logger::get_instance().write_log(message_level, source_file,
                                     line_number, message_buffer.str());
    }

    // Stream interface for chaining operations
    std::ostringstream& stream() { return message_buffer; }

private:
    // Safe formatted string implementation
    void format_message(const char* format, va_list args) {
        va_list argsCopy;
        va_copy(argsCopy, args);

        // Determine required buffer size
        int length = vsnprintf(nullptr, 0, format, argsCopy);
        va_end(argsCopy);

        if (length <= 0) return;

        // Create buffer and format message
        std::vector<char> buffer(length + 1);
        vsnprintf(buffer.data(), buffer.size(), format, args);
        message_buffer << buffer.data();
    }

    log_level message_level;
    const char* source_file;
    int line_number;
    std::ostringstream message_buffer;
};

// Macro helpers for log interface generation
#define LOG_BASE(level) \
    if (level < cppev::logger::get_instance().get_log_level()) ; \
    else cppev::log_message(level, __FILE__, __LINE__).stream()

#define LOG_FMT_BASE(level, format, ...) \
    if (level < cppev::logger::get_instance().get_log_level()) ; \
    else cppev::log_message(level, __FILE__, __LINE__, format, ##__VA_ARGS__)

}   // namespace cppev

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
