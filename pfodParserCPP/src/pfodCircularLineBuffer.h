/**
 * pfodCircularLineBuffer.h
 *
 * Circular buffer of complete lines, e.g. CSV lines, where a partial line is wrong data.
 *  - A line is only available to read once it is complete (its '\n' has been written).
 *    Readers (range or cursor) only see data up to the end of the last complete line.
 *  - When the buffer is full, the oldest complete line is dropped as a whole.
 *  - A line too long for the buffer (more than bufferSize - 1 bytes, including its '\n')
 *    is never served: once it fills the buffer its bytes are discarded, and the rest of
 *    it, up to and including its '\n', is ignored. The line PFOD_LINE_DROPPED_MARKER
 *    ("**line_dropped**\r\n") is stored in its place, so readers can see a line was
 *    lost. Older lines dropped to make room for it while it was being written are not
 *    restored.
 *  - The buffer size is at least PFOD_CIRCULAR_LINE_BUFFER_MIN_SIZE, so it can always
 *    hold the marker line.
 * Readers should read whole lines, with nextLineLength() / readLine() / skipLine(), so
 * they never read part of a line. A reader that cannot store the next line (too long for
 * its own buffer) should skipLine() it and write PFOD_LINE_DROPPED_MARKER instead.
 * The Stream methods available() / read() / peek() still read byte by byte.
 * Implements pfodStream interface for reading data.
 *
 * Default buffer size: 4096 bytes (configurable)
 * Line terminators: '\n' (bare) or '\r\n', a '\r\n' pair is ONE line ending.
 * A lone '\r' is just an ordinary byte.
 *
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */

#ifndef PFOD_CIRCULAR_LINE_BUFFER_H
#define PFOD_CIRCULAR_LINE_BUFFER_H

#include "pfodStream.h"

// The line stored, or written by a reader, in place of a line that was dropped
// because it was too long for the buffer it was going into.
#define PFOD_LINE_DROPPED_MARKER "**line_dropped**\r\n"
// Its length in bytes, 18
#define PFOD_LINE_DROPPED_MARKER_LEN (sizeof(PFOD_LINE_DROPPED_MARKER) - 1)
// The smallest buffer size allowed. The buffer holds bufferSize - 1 bytes, so this
// holds more than the marker line.
#define PFOD_CIRCULAR_LINE_BUFFER_MIN_SIZE (PFOD_LINE_DROPPED_MARKER_LEN + 2)

/**
 * Structure representing a range of bytes in the circular buffer.
 * Used to specify data positions without copying.
 */
struct BufferRange {
    size_t startPos;      // Starting byte position in buffer
    size_t endPos;        // Ending byte position in buffer (exclusive)
};

/**
 * Opaque cursor representing a resumable read position within a
 * pfodCircularLineBuffer, for use with getReadCursor()/setReadCursor().
 * Treat as opaque: obtain it only from getReadCursor() (or the
 * PFOD_CURSOR_START sentinel below) on a given buffer instance, and pass
 * it back only into setReadCursor() on that SAME instance -- never
 * fabricate one, and never reuse a cursor captured from a different
 * instance. Several readers can each hold their own cursor into the same
 * buffer and read at their own separate rates.
 *   pos               -- absolute write-count position (a count of all bytes
 *                        ever written, not a wrapped buffer index), so
 *                        setReadCursor() can tell EXACTLY whether the data at
 *                        the cursor has since been dropped.
 *   totalBytesWritten -- a copy of the buffer's running write counter when
 *                        this cursor was captured.
 *   valid             -- false only for PFOD_CURSOR_START.
 *
 * Contract: call getReadCursor() again immediately after every
 * setReadCursor() so no read progress is lost.
 *
 * The buffer's write counter is never artificially rebased (that would shift
 * the numbering out from under other readers' outstanding cursors). It is left
 * to overflow (wrap around 0) naturally over a long enough lifetime -- unsigned
 * overflow is well defined in C++ -- and setReadCursor() handles that, as it
 * decides eviction from (counter now - cursor.pos), plain unsigned subtraction,
 * which gives the true elapsed byte count across a wrap, as TCP does for its
 * sequence numbers.
 */
struct pfodReadCursor {
    size_t pos;
    size_t totalBytesWritten;
    bool valid; // false only for PFOD_CURSOR_START
};

/**
 * Sentinel pfodReadCursor value meaning "no previous cursor" -- pass
 * this to setReadCursor() to start reading from the current oldest
 * available byte (the same starting point resetForRead() uses).
 */
static const pfodReadCursor PFOD_CURSOR_START = { 0, 0, false };

/**
 * pfodCircularLineBuffer - Line-aware circular buffer for CSV data streaming
 *
 * Only complete lines are served, the oldest complete line is dropped when full,
 * see the top of this file.
 *
 * Implements pfodStream interface for reading data.
 */
class pfodCircularLineBuffer : public pfodStream {
public:
    /**
     * Constructor - Initializes empty circular buffer with specified size.
     * It holds at most bufferSize - 1 bytes. bufferSize should be larger than the
     * longest line; longer lines are never served (see the top of this file).
     *
     * @param bufferSize Size of buffer in bytes (default: 4096). A size less than
     *                   PFOD_CIRCULAR_LINE_BUFFER_MIN_SIZE is increased to that size,
     *                   see getBufferSize().
     */
    pfodCircularLineBuffer(size_t bufferSize = 4096);

    /**
     * Destructor - Cleans up buffer memory
     */
    ~pfodCircularLineBuffer();

    /**
     * Adds a complete line to the buffer (must include its own '\n' or '\r\n'
     * terminator). Uses pfodPrint interface to write characters. If the buffer
     * fills, the oldest data is dropped as described at the top of this file.
     *
     * @param line String containing complete line with '\n' or '\r\n' terminator
     * @return Number of bytes written
     */
    size_t addLine(const char* line);

    /**
     * Gets buffer range for all available data.
     *
     * @param range Output parameter filled with start/end byte positions
     */
    void getAllRange(BufferRange& range);

    /**
     * Sets the current read position to a specific buffer range.
     * Subsequent pfodStream read operations will read from this range.
     *
     * @param range BufferRange specifying what to read
     */
    void setReadRange(const BufferRange& range);

    /**
     * Resets read position to beginning of all available data.
     * Convenience method that calls getAllRange() and setReadRange().
     * Subsequent pfodStream read operations will read from start of buffer.
     */
    void resetForRead();

    /**
     * Length of the next complete line in the current read range, including its
     * '\n' (and '\r' for '\r\n'), so a reader can check it has room for the whole
     * line before reading it. Does not move the read position.
     * If something has already read part of the line with read(), this is the length
     * of the rest of it.
     *
     * @return the length of the next line, or 0 if there is no complete line to read
     */
    size_t nextLineLength();

    /**
     * Reads the next complete line, whole, writing it to out, and moves the read
     * position past it. Check nextLineLength() first if out has limited room.
     *
     * @param out where to write the line
     * @return the length of the line read, or 0 if there is no complete line to read
     */
    size_t readLine(pfodPrint& out);

    /**
     * Moves the read position past the next complete line without reading it, e.g.
     * for a line too long for the reader's own buffer. The reader should then write
     * PFOD_LINE_DROPPED_MARKER in its place.
     *
     * @return the length of the line skipped, or 0 if there is no complete line to read
     */
    size_t skipLine();

    /**
     * Resumes reading from a previously captured cursor, through to the end of
     * the CURRENTLY newest complete line (a partial line being written is not
     * served) -- "catch me up from where I left off". Safe to call repeatedly,
     * forever, even across long gaps during which some of the data available
     * when `cursor` was captured may have been dropped -- the comparison is
     * exact (absolute write counts, not wrapped positions).
     *
     * If `cursor` is PFOD_CURSOR_START, or the data at `cursor` has since been
     * dropped, starts instead from the current oldest available byte (same
     * starting point resetForRead() uses) -- so no data is skipped, though some
     * may be redelivered if it was dropped while nobody was reading.
     *
     * Shares the same read position state as setReadRange()/resetForRead() --
     * do not mix cursor-based and range-based reads on the same instance for two
     * different logical read sequences; whichever call happened most recently wins.
     *
     * @param cursor A value previously returned by getReadCursor() on this SAME
     *               instance, or PFOD_CURSOR_START.
     */
    void setReadCursor(pfodReadCursor cursor);

    /**
     * Captures the current read position for passing to setReadCursor() later
     * (typically kept in a caller-owned variable across many separate calls) to
     * resume reading from here onward. Call AFTER reading whatever is currently
     * available.
     *
     * @return Opaque cursor representing the current read position.
     */
    pfodReadCursor getReadCursor() const;

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
     * Gets number of bytes that can be written without dropping any data.
     * The buffer holds at most bufferSize - 1 bytes (head == tail means empty),
     * so this is bufferSize - 1 - getUsedBytes(). A line of this many bytes,
     * including its '\n', fits without dropping the oldest line.
     *
     * @return Bytes available for new data
     */
    size_t getAvailableBytes() const { return bufferSize - 1 - getUsedBytes(); }

    /**
     * Checks if buffer has no complete lines to read.
     *
     * @return true if buffer contains no complete lines
     */
    bool isEmpty() const { return tail == lineEndPos; }

    /**
     * Clears all data from buffer.
     */
    void clear();

    /**
     * Prints debug information about buffer state and positions.
     * Outputs buffer range, stream positions, and usage stats.
     *
     * @param outPtr Pointer to pfodPrint object for output (e.g., &Serial). Returns silently if null.
     */
    void debugBufferRange(pfodPrint* outPtr);

    // pfodStream interface implementation (read operations)

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

    // pfodPrint interface implementation (write operations for adding data)

    /**
     * Write single byte to buffer. Used by addLine() via pfodPrint interface.
     * If the buffer is full, first drops the oldest complete line; if the line being
     * written fills the buffer by itself it is discarded (see the top of this file).
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
    size_t lineEndPos;        // Position after the last complete line, readers only see up to here
    bool discardingLine;      // true while ignoring the rest of a line too long for the buffer

    // Current read range for pfodStream interface
    size_t readPos;           // Current read position within readRange
    size_t readEndPos;        // End position of current read range

    // Count of every byte ever stored, used ONLY by getReadCursor()/setReadCursor()
    // to give pfodReadCursor an exact, non-aliasing position -- see pfodReadCursor's
    // doc comment. Never artificially rebased, left to wrap at size_t's own width.
    // The one exception: when write() discards a line too long for the buffer it
    // moves head back to lineEndPos and takes those discarded bytes off again, so
    // totalBytesWritten always matches head. That cannot affect any outstanding
    // cursor, as readers never see past lineEndPos, so no cursor points into the
    // discarded bytes.
    size_t totalBytesWritten;

    /**
     * Stores PFOD_LINE_DROPPED_MARKER as a complete line. Only called when the buffer
     * has just been emptied of a too long line, so there is always room.
     */
    void storeDroppedMarker();

    /**
     * Drops the oldest complete line, from tail up to and including its '\n',
     * moving any read positions on the dropped bytes along with tail.
     * Only call when there is a complete line (tail != lineEndPos).
     */
    void dropOldestLine();

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
