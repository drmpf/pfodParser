/**
 * pfodCircularLineBuffer.h
 *
 * Line-aware circular buffer that stores complete lines only.
 * When wrapping, automatically adjusts to maintain line boundaries.
 * Implements Stream interface for reading data.
 *
 * Default buffer size: 8KB (configurable)
 * Line terminator: \r\n
 *
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 */

#ifndef PFOD_CIRCULAR_LINE_BUFFER_H
#define PFOD_CIRCULAR_LINE_BUFFER_H

#include <Arduino.h>

/**
 * Structure representing a range of bytes in the circular buffer.
 * Used to specify data positions without copying.
 */
struct BufferRange {
    size_t startPos;      // Starting byte position in buffer
    size_t endPos;        // Ending byte position in buffer (exclusive)
    size_t startLine;     // Line number at start position
    size_t endLine;       // Line number at end position
};

/**
 * pfodCircularLineBuffer - Line-aware circular buffer for CSV data streaming
 *
 * Maintains complete lines only, automatically adjusting start position
 * when buffer wraps to ensure no partial lines. Tracks line counts for
 * efficient retrieval by line number.
 *
 * Implements Arduino Stream interface for reading data.
 */
class pfodCircularLineBuffer : public Stream {
public:
    /**
     * Constructor - Initializes empty circular buffer with specified size.
     * buffersize must be > line lenght, 
     * if buffersize is < line length no data will be available to read
     *
     * @param bufferSize Size of buffer in bytes (default: 4096)
     */
    pfodCircularLineBuffer(size_t bufferSize = 4096);

    /**
     * Destructor - Cleans up buffer memory
     */
    ~pfodCircularLineBuffer();

    /**
     * Adds a complete line to the buffer (must include \r\n terminator).
     * Uses Print interface to write characters. If wrapping occurs and
     * overwrites start position, adjusts start to next complete line.
     *
     * @param line String containing complete line with \r\n terminator
     * @return Number of bytes written
     */
    size_t addLine(const char* line);

    /**
     * Adds a complete line to the buffer (must include \r\n terminator).
     * Uses Print interface to write characters. If wrapping occurs and
     * overwrites start position, adjusts start to next complete line.
     *
     * @param line String containing complete line with \r\n terminator
     * @return Number of bytes written
     */
    size_t addLine(const String& line);

    /**
     * Gets buffer range starting from specified line number.
     * Returns data from that line until end of buffer.
     * If requested line is older than available, returns from earliest line.
     *
     * @param fromLineCount Line number to start from
     * @param range Output parameter filled with start/end byte positions and line counts
     */
    void getRange(size_t fromLineCount, BufferRange& range);

    /**
     * Gets buffer range for all available data.
     *
     * @param range Output parameter filled with start/end byte positions and line counts
     */
    void getAllRange(BufferRange& range);

    /**
     * Sets the current read position to a specific buffer range.
     * Subsequent Stream read operations will read from this range.
     *
     * @param range BufferRange specifying what to read
     */
    void setReadRange(const BufferRange& range);

    /**
     * Resets read position to beginning of all available data.
     * Convenience method that calls getAllRange() and setReadRange().
     * Subsequent Stream read operations will read from start of buffer.
     */
    void resetForRead();

    /**
     * Gets the current start line number (oldest available line).
     *
     * @return Line number of first available line in buffer
     */
    size_t getStartLineCount() const { return startLineCount; }

    /**
     * Gets the current end line number (newest line + 1).
     *
     * @return Line number after last line in buffer
     */
    size_t getEndLineCount() const { return endLineCount; }

    /**
     * Gets total number of lines currently in buffer.
     *
     * @return Number of complete lines stored
     */
    size_t getLineCount() const { return endLineCount - startLineCount; }

    /**
     * Gets the configured buffer size.
     *
     * @return Total buffer size in bytes
     */
    size_t getBufferSize() const { return bufferSize; }

    /**
     * Gets number of bytes currently used in buffer.
     *
     * @return Bytes of data stored
     */
    size_t getUsedBytes() const;

    /**
     * Gets number of bytes available in buffer.
     *
     * @return Bytes available for new data
     */
    size_t getAvailableBytes() const { return bufferSize - getUsedBytes(); }

    /**
     * Checks if buffer is empty.
     *
     * @return true if buffer contains no data
     */
    bool isEmpty() const { return head == tail; }

    /**
     * Clears all data from buffer and resets line counts.
     */
    void clear();

    /**
     * Prints debug information about buffer state and positions.
     * Outputs buffer range, stream positions, line counts, and usage stats.
     *
     * @param outPtr Pointer to Print object for output (e.g., &Serial). Returns silently if null.
     */
    void debugBufferRange(Print* outPtr);

    // Stream interface implementation (read operations)

    /**
     * Returns number of bytes available to read from current read range.
     *
     * @return Number of bytes available
     */
    virtual int available() override;

    /**
     * Reads one byte from current read range without advancing position.
     *
     * @return Byte value (0-255) or -1 if no data available
     */
    virtual int peek() override;

    /**
     * Reads one byte from current read range and advances position.
     *
     * @return Byte value (0-255) or -1 if no data available
     */
    virtual int read() override;

    // Print interface implementation (write operations for adding data)

    /**
     * Write single byte to buffer. Used by addLine() via Print interface.
     * Automatically wraps and adjusts start position when \r\n detected.
     *
     * @param c Byte to write
     * @return 1 if written, 0 if failed
     */
    virtual size_t write(uint8_t c) override;

    /**
     * Write buffer. Calls write(uint8_t) for each byte.
     *
     * @param buffer Data to write
     * @param size Number of bytes
     * @return Number of bytes written
     */
    virtual size_t write(const uint8_t *buffer, size_t size) override;

private:
    size_t bufferSize;        // Configured buffer size
    uint8_t* buffer;          // Circular buffer storage
    size_t head;              // Write position (next byte to write)
    size_t tail;              // Read position (first byte to read)
    size_t startLineCount;    // Line number at tail position
    size_t endLineCount;      // Line number at head position (next line to write)

    // Current read range for Stream interface
    size_t readPos;           // Current read position within readRange
    size_t readEndPos;        // End position of current read range
    size_t lineByteCount;
    uint8_t prevByte;


    /**
     * Finds the next complete line starting from given position.
     * Searches for \r\n terminator.
     *
     * @param fromPos Starting position to search
     * @param lineEnd Output parameter for position after \r\n
     * @return true if complete line found, false if no \r\n found
     */
    bool findNextLine(size_t fromPos, size_t& lineEnd);


    /**
     * Calculates number of bytes between two positions, handling wraparound.
     *
     * @param start Starting position
     * @param end Ending position
     * @return Number of bytes from start to end
     */
    size_t bytesInRange(size_t start, size_t end) const;
};

#endif // PFOD_CIRCULAR_LINE_BUFFER_H
