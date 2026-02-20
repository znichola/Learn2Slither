/**
 * WASM Output Parser for C++ Logger - for mirror see /include/reader.hpp
 * 
 * Parses structured messages from C++ stdout/stderr that follow the pattern:
 * TYPE_START[
 * content
 * ]TYPE_END
 */

class WasmOutputParser {
    constructor(messageTypes = []) {
        this.stdoutBuffer = '';
        this.stderrBuffer = '';

        if (messageTypes.length === 0) {
            throw new Error('At least one message type must be provided');
        }

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

// Export for use in browser or Node.js
if (typeof module !== 'undefined' && module.exports) {
    module.exports = { WasmOutputParser, createModuleWithParser };
}