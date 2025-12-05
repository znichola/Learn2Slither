#pragma once
#include <iostream>
#include <sstream>
#include <string>

struct LoggerConfig {
    bool enableBoardOutput = true;
    bool enableStdoutLogs = true;
    bool enableErrors = true;
};

class Logger {
private:
    enum class MessageType { Board, Log, Error };

    struct MessageSpec {
        const char* startMarker;
        const char* endMarker;
        std::ostream* stream;
        bool enabled;
    };

    class LogStream {
    public:
        LogStream(Logger* logger, MessageType type)
            : logger_(logger), type_(type) {}

        ~LogStream() {
            logger_->flush(type_, ss_.str());
        }

        template<typename T>
        LogStream& operator<<(const T& value) {
            ss_ << value;
            return *this;
        }

        LogStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
            manip(ss_);
            return *this;
        }

    private:
        Logger* logger_;
        MessageType type_;
        std::stringstream ss_;
    };

public:
    LoggerConfig config_;

    Logger(const LoggerConfig& config = LoggerConfig())
        : config_(config) {}

    LogStream board() {
        return LogStream(this, MessageType::Board);
    }

    LogStream log() {
        return LogStream(this, MessageType::Log);
    }

    LogStream error() {
        return LogStream(this, MessageType::Error);
    }

private:
    MessageSpec getSpec(MessageType type) const {
        switch(type) {
            case MessageType::Board:
                return {"BOARD_START[", "]BOARD_END",
                        &std::cout, config_.enableBoardOutput};
            case MessageType::Log:
                return {"LOG_START[", "]LOG_END",
                        &std::cout, config_.enableStdoutLogs};
            case MessageType::Error:
                return {"ERROR_START[", "]ERROR_END",
                        &std::cerr, config_.enableErrors};
        }
        return {"", "", &std::cout, false}; // Should never reach
    }

    void flush(MessageType type, const std::string& message) {
        if (message.empty()) return;

        MessageSpec spec = getSpec(type);
        if (!spec.enabled) return;

        // Trim trailing newlines
        std::string trimmed_message = message;
        while (!trimmed_message.empty() && 
            (trimmed_message.back() == '\n' || trimmed_message.back() == '\r')) {
            trimmed_message.pop_back();
        }

        *spec.stream << spec.startMarker << std::endl;
        *spec.stream << trimmed_message;
        *spec.stream << std::endl << spec.endMarker << std::endl;
        spec.stream->flush();
    }

    friend class LogStream;
};