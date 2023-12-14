//
// Created by -Ry on 14/12/2023.
//

#ifndef OCTVIS_LOGGING_H
#define OCTVIS_LOGGING_H

#include <string>
#include <format>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <chrono>

//############################################################################//
// | LOGGING MACROS |
//############################################################################//

namespace _octvis_logging {

    std::string get_time_formatted(const char* fmt = "%H:%M:%S") {

        using Clock = std::chrono::system_clock;
        std::time_t current_time = Clock::to_time_t(Clock::now());
        std::tm tm_time = *std::localtime(&current_time);

        std::stringstream ss{};
        ss.str().reserve(32);
        ss << std::put_time(&tm_time, fmt);

        return ss.str();
    }

    void check_assertion(
            bool predicate,
            std::string msg,
            const char* predicate_str,
            const char* file,
            const char* function,
            int line
    );

}

#define OCTVIS_LOG(level, out, msg)              \
    out << std::format(                          \
        "{:<8} | {} | {}\n",                     \
        level,                                   \
        ::_octvis_logging::get_time_formatted(), \
        msg                                      \
    )

#define OCTVIS_INFO(msg, ...)  OCTVIS_LOG("INFO" , std::cout, std::vformat(msg, std::make_format_args(__VA_ARGS__)))
#define OCTVIS_TRACE(msg, ...) OCTVIS_LOG("TRACE", std::cout, std::vformat(msg, std::make_format_args(__VA_ARGS__)))
#define OCTVIS_WARN(msg, ...)  OCTVIS_LOG("WARN" , std::cout, std::vformat(msg, std::make_format_args(__VA_ARGS__)))
#define OCTVIS_ERROR(msg, ...) OCTVIS_LOG("ERROR", std::cout, std::vformat(msg, std::make_format_args(__VA_ARGS__)))
#define OCTVIS_DEBUG(msg, ...) OCTVIS_LOG("DEBUG", std::cout, std::vformat(msg, std::make_format_args(__VA_ARGS__)))

#define OCTVIS_ASSERT(predicate, msg, ...)         \
    do {                                           \
        ::_octvis_logging::check_assertion(         \
            !!(predicate),                         \
            std::vformat(                          \
                msg,                               \
                std::make_format_args(__VA_ARGS__) \
            ),                                     \
            #predicate,                            \
            __FILE__,                              \
            __FUNCTION__,                          \
            __LINE__                               \
        );                                         \
    } while (false)

namespace _octvis_logging {
    void check_assertion(
            bool predicate,
            std::string msg,
            const char* predicate_str,
            const char* file,
            const char* function,
            int line
    ) {
        if (!predicate) {
            OCTVIS_ERROR(
                    std::format(
                            "[Assertion Failure]"
                            "\n\tCondition: '{}'"
                            "\n\tMessage  : '{}'"
                            "\n\tFile     : '{}'"
                            "\n\tFunction : '{}'"
                            "\n\tLine     : '{}'",
                            predicate_str,
                            msg,
                            file,
                            function,
                            line
                    )
            );
            assert(false);
        }
    }
}
#endif
