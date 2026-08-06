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

        // Trim trailing newline
        while (!message.empty() && (message.back() == '\n')) {
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

// All message formats need to also be declared in the index.html when initialising the js message parser

inline LogStream board() {
    return {std::cout, "BOARD_START[", "]BOARD_END"};
}

inline LogStream log() {
    return {std::cout, "LOG_START[", "]LOG_END"};
}

inline LogStream error() {
    return {std::cerr, "ERROR_START[", "]ERROR_END"};
}

inline LogStream RUN_DONE() {
    return {std::cout, "RUN_DONE_START[", "]RUN_DONE_END"};
}

inline LogStream save_agent() {
    return {std::cout, "SAVE_AGENT_START[", "]SAVE_AGENT_END"};
}

inline void QTABLE_SIZE(const size_t size) {
    std::cout << "QTABLE_SIZE_START[\n" << size << "\n]QTABLE_SIZE_END";
    return ;
}

inline LogStream PUSH_CONFIG() {
    return {std::cout, "PUSH_CONFIG_START[", "]PUSH_CONFIG_END"};
}

}