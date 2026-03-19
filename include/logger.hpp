#pragma once
#include <iostream>
#include <sstream>
#include <string>

namespace Logger {

class LogStream {
public:
    LogStream(std::ostream &stream, const std::string &start, const std::string &end)
        : _stream(stream), _start(start), _end(end) {}

    ~LogStream() {
        std::string message = _ss.str();

        // Trim trailing newline / carriage return
        while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
            message.pop_back();
        }

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
    std::string _start;
   std::string _end;
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

inline LogStream batch_done() {
    return {std::cout, "BATCH_DONE_START[", "]BATCH_DONE_END"};
}

}