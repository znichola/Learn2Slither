#pragma once

#include <algorithm>
#include <deque>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace Reader {

// ---------------------------------------------------------------------------
// Message types - 1/2
// ---------------------------------------------------------------------------

struct Start { std::string content; };
struct Send { std::string content; };

using Message = std::variant<Start, Send>;

class Parser {
private:
    struct PatternEntry {
        std::string_view startMarker;
        std::string_view endMarker;
        Message (*make)(std::string content);
    };

    // Message Patterns - 2/2
    static constexpr PatternEntry _patterns[] = {
        { "START_START[", "]START_END", [](std::string c) -> Message { return Start{std::move(c)}; } },
        { "SEND_START[", "]SEND_END", [](std::string c) -> Message { return Send{std::move(c)}; } },
    };

public:
    /**
     * Read one chunk from stdin into the buffer, then extract all complete
     * messages into the queue.
     *
     * Returns false when stdin is closed (EOF), true otherwise.
     */
    bool read() {
        char chunk[4096];
        if (!std::cin.read(chunk, sizeof(chunk))) {
            if (std::cin.gcount() > 0)
                _buffer.append(chunk, std::cin.gcount());
            extractAll();
            return false;
        }
        _buffer.append(chunk, std::cin.gcount());
        extractAll();
        return true;
    }

    /**
     * Remove and return the next fully parsed message, or nullopt if the
     * queue is empty.
     */
    std::optional<Message> pop() {
        if (_queue.empty()) return std::nullopt;
        Message msg = std::move(_queue.front());
        _queue.pop_front();
        return msg;
    }

private:
    std::string         _buffer;
    std::deque<Message> _queue;

    void extractAll() {
        std::cout << "Read called, and BUFFER IS" << _buffer << "\n";
        bool modified = true;
        while (modified) {
            modified = false;
            for (const auto& p : _patterns) {
                const size_t startPos = _buffer.find(p.startMarker);
                if (startPos == std::string::npos) continue;

                const size_t contentStart = startPos + p.startMarker.size();
                const size_t endPos = _buffer.find(p.endMarker, contentStart);
                if (endPos == std::string::npos) continue; // incomplete — wait for more chunks

                const size_t rawEnd = endPos + p.endMarker.size();

                std::string content = trim(_buffer.substr(contentStart, endPos - contentStart));
                _queue.push_back(p.make(std::move(content)));

                _buffer.erase(startPos, rawEnd - startPos);
                modified = true;
                break;
            }
        }
    }

    static std::string trim(const std::string& s) {
        const auto isSpace = [](unsigned char c) { return std::isspace(c); };
        const auto front = std::find_if_not(s.begin(), s.end(), isSpace);
        const auto back  = std::find_if_not(s.rbegin(), s.rend(), isSpace).base();
        return (front < back) ? std::string(front, back) : std::string{};
    }
};

}