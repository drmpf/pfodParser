/**
 * pfodCircularLineBuffer.cpp
 *
 * Implementation of line-aware circular buffer for ESP32.
 *
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 */

#include "pfodCircularLineBuffer.h"

/**
 * Constructor - Allocates buffer memory and initializes state.
 * buffersize must be > line lenght, 
 * if buffersize is < line length no data will be available to read
 *
 * @param bufferSize Size of buffer in bytes (default: 4096)
 */
pfodCircularLineBuffer::pfodCircularLineBuffer(size_t bufferSize)
    : bufferSize(bufferSize)
    , buffer(nullptr)
    , head(0)
    , tail(0)
    , startLineCount(0)
    , endLineCount(0)
    , readPos(0)
    , readEndPos(0)
{
    buffer = new uint8_t[bufferSize];
}

/**
 * Destructor - Frees buffer memory.
 */
pfodCircularLineBuffer::~pfodCircularLineBuffer() {
    delete[] buffer;
}

/**
 * Adds a complete line to the buffer (must include \r\n terminator).
 * Uses Print interface write() methods. Automatically manages wrapping
 * by checking after each byte if old data was overwritten.
 *
 * @param line C-string containing complete line with \r\n terminator
 * @return Number of bytes written
 */
size_t pfodCircularLineBuffer::addLine(const char* line) {
    return print(line);
}

/**
 * Adds a complete line to the buffer (must include \r\n terminator).
 * Uses Print interface write() methods. Automatically manages wrapping
 * by checking after each byte if old data was overwritten.
 *
 * @param line Arduino String containing complete line with \r\n terminator
 * @return Number of bytes written
 */
size_t pfodCircularLineBuffer::addLine(const String& line) {
    return print(line);
}

/**
 * Write single byte to buffer. Automatically wraps around buffer end.
 * When \r\n sequence is detected, increments line count and adjusts
 * start position if current line wrapped over the tail.
 *
 * @param c Byte to write
 * @return 1 if written
 */
size_t pfodCircularLineBuffer::write(uint8_t c) {
    buffer[head] = c;
    head = (head + 1) % bufferSize;
    lineByteCount++;

    if (head == tail) {
        startLineCount++;
        do {
            // Check if read pointers are at the position about to be discarded
            // and advance them to stay ahead of the advancing tail
            if (readPos == tail) {
                readPos = (tail + 1) % bufferSize;
            }
            if (readEndPos == tail) {
                readEndPos = (tail + 1) % bufferSize;
            }

            uint8_t lastC = buffer[tail];
            tail = (tail + 1) % bufferSize;
            if (lastC == '\n') {
                break;
            }
        } while (tail != head);
    }

    if (prevByte == '\r' && c == '\n') {
        endLineCount++;

        if (lineByteCount > bufferSize - 1) {
            tail = head;
            startLineCount = endLineCount;
            readPos = head;
            readEndPos = head;
        }

        lineByteCount = 0;
    }

    prevByte = c;
    return 1;
}

/**
 * Write buffer of bytes. Calls write(uint8_t) for each byte.
 *
 * @param buffer Data to write
 * @param size Number of bytes
 * @return Number of bytes written
 */
size_t pfodCircularLineBuffer::write(const uint8_t *buffer, size_t size) {
    size_t written = 0;
    for (size_t i = 0; i < size; i++) {
        written += write(buffer[i]);
    }
    return written;
}

/**
 * Gets buffer range starting from specified line number.
 * If requested line is older than available, returns from earliest line.
 * Returns data from that line until end of buffer (head position).
 *
 * @param fromLineCount Line number to start from
 * @param range Output parameter filled with start/end byte positions and line counts
 */
void pfodCircularLineBuffer::getRange(size_t fromLineCount, BufferRange& range) {
    if (isEmpty()) {
        range.startPos = tail;
        range.endPos = head;
        range.startLine = startLineCount;
        range.endLine = endLineCount;
        return;
    }

    if (fromLineCount < startLineCount) {
        fromLineCount = startLineCount;
    }

    if (fromLineCount >= endLineCount) {
        range.startPos = head;
        range.endPos = head;
        range.startLine = endLineCount;
        range.endLine = endLineCount;
        return;
    }

    size_t linesToSkip = fromLineCount - startLineCount;
    size_t searchPos = tail;
    size_t currentLine = startLineCount;

    for (size_t i = 0; i < linesToSkip; i++) {
        size_t lineEnd;
        if (findNextLine(searchPos, lineEnd)) {
            searchPos = lineEnd;
            currentLine++;
        }
    }

    range.startPos = searchPos;
    range.endPos = head;
    range.startLine = fromLineCount;
    range.endLine = endLineCount;
}

/**
 * Gets buffer range for all available data.
 * Returns entire buffer contents from tail to head.
 *
 * @param range Output parameter filled with start/end byte positions and line counts
 */
void pfodCircularLineBuffer::getAllRange(BufferRange& range) {
    range.startPos = tail;
    range.endPos = head;
    range.startLine = startLineCount;
    range.endLine = endLineCount;
}

/**
 * Sets the current read position to a specific buffer range.
 * Subsequent Stream read operations will read from this range.
 *
 * @param range BufferRange specifying what to read
 */
void pfodCircularLineBuffer::setReadRange(const BufferRange& range) {
    readPos = range.startPos;
    readEndPos = range.endPos;
}

/**
 * Resets read position to beginning of all available data.
 * Convenience method that calls getAllRange() and setReadRange().
 * Subsequent Stream read operations will read from start of buffer.
 */
void pfodCircularLineBuffer::resetForRead() {
    BufferRange range;
    getAllRange(range);
    setReadRange(range);
}

/**
 * Returns number of bytes available to read from current read range.
 * Implements Stream::available()
 *
 * @return Number of bytes available in current read range
 */
int pfodCircularLineBuffer::available() {
    return (int)bytesInRange(readPos, readEndPos);
}

/**
 * Reads one byte from current read range without advancing position.
 * Implements Stream::peek()
 *
 * @return Byte value (0-255) or -1 if no data available
 */
int pfodCircularLineBuffer::peek() {
    if (readPos == readEndPos) {
        return -1;
    }

    return buffer[readPos];
}

/**
 * Reads one byte from current read range and advances position.
 * Implements Stream::read()
 *
 * @return Byte value (0-255) or -1 if no data available
 */
int pfodCircularLineBuffer::read() {
    if (readPos == readEndPos) {
        return -1;
    }

    uint8_t byte = buffer[readPos];
    readPos = (readPos + 1) % bufferSize;

    return byte;
}

/**
 * Gets number of bytes currently used in buffer.
 * Handles wraparound case where head < tail.
 *
 * @return Bytes of data stored
 */
size_t pfodCircularLineBuffer::getUsedBytes() const {
    if (head >= tail) {
        return head - tail;
    }
    return bufferSize - tail + head;
}

/**
 * Clears all data from buffer and resets line counts.
 * Does not free memory, just resets pointers and counters.
 */
void pfodCircularLineBuffer::clear() {
    head = 0;
    tail = 0;
    startLineCount = 0;
    endLineCount = 0;
    readPos = 0;
    readEndPos = 0;
    lineByteCount = 0;
    prevByte = 0;
}

/**
 * Prints debug information about buffer state and positions.
 * Outputs buffer range, stream positions, line counts, and usage stats.
 *
 * @param outPtr Pointer to Print object for output. Returns silently if null.
 */
void pfodCircularLineBuffer::debugBufferRange(Print* outPtr) {
    if (!outPtr) {
        return;
    }

    outPtr->println("=== pfodCircularLineBuffer Debug ===");

    outPtr->print("Buffer size: ");
    outPtr->println(bufferSize);

    outPtr->print("Used bytes: ");
    outPtr->print(getUsedBytes());
    outPtr->print(" / Available: ");
    outPtr->println(getAvailableBytes());

    outPtr->println();
    outPtr->println("Buffer positions:");
    outPtr->print("  head (write): ");
    outPtr->println(head);
    outPtr->print("  tail (read):  ");
    outPtr->println(tail);
    outPtr->print("  isEmpty: ");
    outPtr->println(isEmpty() ? "true" : "false");

    outPtr->println();
    outPtr->println("Line counts:");
    outPtr->print("  startLineCount: ");
    outPtr->println(startLineCount);
    outPtr->print("  endLineCount:   ");
    outPtr->println(endLineCount);
    outPtr->print("  total lines:    ");
    outPtr->println(getLineCount());

    outPtr->println();
    outPtr->println("Stream read positions:");
    outPtr->print("  readPos:    ");
    outPtr->println(readPos);
    outPtr->print("  readEndPos: ");
    outPtr->println(readEndPos);
    outPtr->print("  available:  ");
    outPtr->println(available());

    outPtr->println();
    outPtr->print("Buffer contents from tail(");
    outPtr->print(tail);
    outPtr->print(") to head-1(");
    outPtr->print((head > 0) ? head - 1 : bufferSize - 1);
    outPtr->println(") [decimal]:");

    if (isEmpty()) {
        outPtr->println("  (empty)");
    } else {
        size_t pos = tail;
        size_t count = 0;

        while (pos != head) {
            if (count % 20 == 0) {
                if (count > 0) {
                    outPtr->println();
                }
                outPtr->print("  [");
                outPtr->print(pos);
                outPtr->print("]: ");
            }

            outPtr->print(buffer[pos]);

            if (buffer[pos] == 13) {
                outPtr->print("(\\r)");
            } else if (buffer[pos] == 10) {
                outPtr->print("(\\n)");
            }

            outPtr->print(" ");

            count++;
            pos = (pos + 1) % bufferSize;
        }
        outPtr->println();
        outPtr->print("  Total bytes: ");
        outPtr->println(count);
    }

    outPtr->println("====================================");
}

/**
 * Finds the next complete line starting from given position.
 * Searches for \r\n terminator in circular buffer.
 *
 * @param fromPos Starting position to search
 * @param lineEnd Output parameter for position after \r\n
 * @return true if complete line found, false if no \r\n found
 */
bool pfodCircularLineBuffer::findNextLine(size_t fromPos, size_t& lineEnd) {
    size_t pos = fromPos;

    while (pos != head) {
        if (buffer[pos] == '\r') {
            size_t nextPos = (pos + 1) % bufferSize;
            if (nextPos != head && buffer[nextPos] == '\n') {
                lineEnd = (nextPos + 1) % bufferSize;
                return true;
            }
        }
        pos = (pos + 1) % bufferSize;
    }

    return false;
}


/**
 * Calculates number of bytes between two positions, handling wraparound.
 * If end < start, assumes buffer has wrapped around.
 *
 * @param start Starting position
 * @param end Ending position
 * @return Number of bytes from start to end
 */
size_t pfodCircularLineBuffer::bytesInRange(size_t start, size_t end) const {
    if (end >= start) {
        return end - start;
    }
    return bufferSize - start + end;
}
