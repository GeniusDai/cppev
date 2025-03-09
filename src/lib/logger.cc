#include "cppev/logger.h"

namespace cppev
{

logger &logger::get_instance()
{
    static logger instance;
    return instance;
}

void logger::set_log_level(log_level level)
{
    std::lock_guard<std::mutex> lock(mtx_);
    current_level_ = level;
}

void logger::add_output_stream(std::ostream &output)
{
    std::lock_guard<std::mutex> lock(mtx_);
    output_streams_.push_back(&output);
}

log_level logger::get_log_level() const
{
    return current_level_;
}

void logger::write_log(log_level level, const std::string &file, int line,
                       const std::string &message)
{
    if (level < current_level_)
    {
        return;
    }
    std::stringstream log_entry;
    add_color(log_entry, level);
    add_timestamp(log_entry);
    add_thread_id(log_entry);
    log_entry << " [" << level_to_string(level) << "] ";
    log_entry << "[" << file << ":" << line << "] ";
    log_entry << message << std::endl;
    reset_color(log_entry);

    std::lock_guard<std::mutex> lock(mtx_);
    for (auto &stream : output_streams_)
    {
        if (stream)
        {
            *stream << log_entry.str();
            stream->flush();
        }
    }
}

logger::logger() : current_level_(log_level::info)
{
    output_streams_.push_back(&std::cout);
}

std::string logger::level_to_string(log_level level) const
{
    switch (level)
    {
    case log_level::debug:
        return "DEBUG";
    case log_level::info:
        return "INFO";
    case log_level::warning:
        return "WARNING";
    case log_level::error:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

void logger::add_color(std::ostream &os, log_level level)
{
    switch (level)
    {
    case log_level::debug:
        os << DEBUG_COLOR;
        break;
    case log_level::info:
        os << INFO_COLOR;
        break;
    case log_level::warning:
        os << WARNING_COLOR;
        break;
    case log_level::error:
        os << ERROR_COLOR;
        break;
    default:
        break;
    }
}

void logger::add_timestamp(std::ostream &os)
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    os << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << '.'
       << std::setfill('0') << std::setw(3) << ms.count();
}

void logger::add_thread_id(std::ostream &os)
{
    os << " [Thread:";
#ifdef __linux__
    os << "0x";
#endif
    os << std::hex << std::this_thread::get_id() << std::dec << "]";
}

void logger::reset_color(std::ostream &os)
{
    os << RESET_COLOR;
}

log_message::log_message(log_level level, const char *file, int line)
    : message_level_(level), source_file_(file), line_number_(line)
{
}

log_message::log_message(log_level level, const char *file, int line,
                         const char *format, ...)
    : message_level_(level), source_file_(file), line_number_(line)
{
    va_list args;
    va_start(args, format);
    format_message(format, args);
    va_end(args);
}

log_message::~log_message()
{
    logger::get_instance().write_log(message_level_, source_file_, line_number_,
                                     message_buffer_.str());
}

std::ostringstream &log_message::stream()
{
    return message_buffer_;
}

void log_message::format_message(const char *format, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);

    // Determine required buffer size
    int length = vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);

    if (length <= 0)
    {
        return;
    }

    // Create buffer and format message
    std::vector<char> buffer(length + 1);
    vsnprintf(buffer.data(), buffer.size(), format, args);
    message_buffer_ << buffer.data();
}

}  // namespace cppev
