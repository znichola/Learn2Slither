#pragma once
#include <iostream>
#include <sstream>
#include <string_view>

namespace Logger {

class LogStream {
public:
    LogStream(std::ostream &stream, std::string_view start, std::string_view end)
        : _stream(stream), _start(start), _end(end) {}

    ~LogStream() {
        std::string_view message = _ss.view();

        while (!message.empty() && (message.back() == '\n' || message.back() == '\r'))
            message.remove_suffix(1);

        if (message.empty())
            return;

        _stream << _start << '\n' << message << '\n' << _end << '\n';
        _stream.flush();
    }

    template <typename T>
    LogStream &operator<<(const T &value) {
        _ss << value;
        return *this;
    }

    LogStream &operator<<(std::ostream &(*manip)(std::ostream &)) {
        manip(_ss);
        return *this;
    }

private:
    std::ostream &_stream;
    std::string_view _start;
    std::string_view _end;
    std::ostringstream _ss;
};

inline LogStream board() {
    return {std::cout, "BOARD_START[", "]BOARD_END"};
}

inline LogStream log() {
    return {std::cout, "LOG_START[", "]LOG_END"};
}

inline LogStream error() {
    return {std::cerr, "ERROR_START[", "]ERROR_END"};
}

inline LogStream episode_done() {
    return {std::cout, "EPISODE_DONE_START[", "]EPISODE_DONE_END"};
}

}