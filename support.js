/**
 * WASM Output Parser for C++ Logger
 * 
 * Parses structured messages from C++ stdout/stderr that follow the pattern:
 * TYPE_START[
 * content
 * ]TYPE_END
 */

class WasmOutputParser {
    constructor(messageTypes = ['BOARD', 'LOG', 'ERROR']) {
        this.stdoutBuffer = '';
        this.stderrBuffer = '';

        // Derive patterns from message types
        this.patterns = this.derivePatterns(messageTypes);
    }

    /**
     * Derive message patterns from type names
     * Pattern: TYPE_START[ ... ]TYPE_END
     */
    derivePatterns(types) {
        const patterns = {};

        for (const type of types) {
            const upperType = type.toUpperCase();
            const lowerType = type.toLowerCase();

            patterns[lowerType] = {
                start: new RegExp(`${upperType}_START\\[`),
                end: new RegExp(`\\]${upperType}_END`),
                name: lowerType
            };
        }

        return patterns;
    }

    /**
     * Process stdout chunk from WASM
     */
    processStdout(chunk) {
        this.stdoutBuffer += chunk;
        this.extractMessages(this.stdoutBuffer, 'stdout', (remaining) => {
            this.stdoutBuffer = remaining;
        });
    }

    /**
     * Process stderr chunk from WASM
     */
    processStderr(chunk) {
        this.stderrBuffer += chunk;
        this.extractMessages(this.stderrBuffer, 'stderr', (remaining) => {
            this.stderrBuffer = remaining;
        });
    }

    /**
     * Extract complete messages from buffer
     */
    extractMessages(buffer, source, updateBuffer) {
        let modified = true;
        let workingBuffer = buffer;

        while (modified) {
            modified = false;

            // Try to find and extract each message type
            for (const [type, pattern] of Object.entries(this.patterns)) {
                const startMatch = workingBuffer.match(pattern.start);

                if (startMatch) {
                    const startIdx = startMatch.index;
                    const contentStart = startIdx + startMatch[0].length;
                    const endMatch = workingBuffer.slice(contentStart).match(pattern.end);

                    if (endMatch) {
                        // Extract the content between markers
                        const content = workingBuffer.slice(contentStart, contentStart + endMatch.index);

                        // Emit the parsed message
                        this.onMessage({
                            type: pattern.name,
                            source: source,
                            content: content.trim(),
                            raw: workingBuffer.slice(startIdx, contentStart + endMatch.index + endMatch[0].length)
                        });

                        // Remove the processed message from buffer
                        workingBuffer = workingBuffer.slice(0, startIdx) +
                            workingBuffer.slice(contentStart + endMatch.index + endMatch[0].length);
                        modified = true;
                        break; // Restart loop to process next message
                    }
                }
            }
        }

        updateBuffer(workingBuffer);
    }

    /**
     * Override this method or replace it to handle parsed messages
     */
    onMessage(message) {
        console.log(`[${message.type.toUpperCase()}]`, message.content);
    }

    /**
     * Clear all buffers (useful for reset)
     */
    clear() {
        this.stdoutBuffer = '';
        this.stderrBuffer = '';
    }
}

/**
 * Setup Emscripten Module with output parser
 */
function createModuleWithParser(messageTypes) {
    const parser = new WasmOutputParser(messageTypes);

    // You can customize message handling
    parser.onMessage = (message) => {
        switch (message.type) {
            case 'board':
                console.log('%c[BOARD]', 'color: green; font-weight: bold');
                console.log(message.content);
                break;
            case 'log':
                console.log('%c[LOG]', 'color: blue', message.content);
                break;
            case 'error':
                console.error('%c[ERROR]', 'color: red; font-weight: bold', message.content);
                break;
            default:
                console.log(`[${message.type.toUpperCase()}]`, message.content);
        }
    };

    const Module = {
        print: (text) => {
            parser.processStdout(text + '\n');
        },
        printErr: (text) => {
            parser.processStderr(text + '\n');
        },
        onRuntimeInitialized: () => {
            console.log('WASM Runtime initialized');
        }
    };

    return { Module, parser };
}

// Export for use in browser or Node.js
if (typeof module !== 'undefined' && module.exports) {
    module.exports = { WasmOutputParser, createModuleWithParser };
}