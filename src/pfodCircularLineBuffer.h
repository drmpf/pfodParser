/**
 * pfodCircularLineBuffer.h
 *
 * Line-aware circular buffer that stores complete lines only.
 * When wrapping, automatically adjusts to maintain line boundaries.
 * Implements Stream interface for reading data.
 *
 * Default buffer size: 8KB (configurable)
 * Line terminators: '\n' (bare) or '\r\n' -- a '\r\n' pair counts as ONE
 * line ending, never two. A lone '\r' (not immediately followed by '\n')
 * is NOT treated as a line ending at all -- it's just an ordinary byte,
 * with no effect on line counting or on where wrapping stops. This
 * matches modern text (Unix/macOS bare '\n', Windows/most text network
 * protocols '\r\n') and deliberately does not support the old
 * Classic-Mac-OS convention of a lone '\r' as its own line ending, which
 * is legacy-only today (changed 2026-09-21, per explicit instruction --
 * see pfodParserAI-guide.md section 6.2 for the full reasoning).
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
 * Opaque cursor representing a resumable read position within a
 * pfodCircularLineBuffer, for use with getReadCursor()/setReadCursor().
 * Treat as opaque: obtain it only from getReadCursor() (or the
 * PFOD_CURSOR_START sentinel below) on a given buffer instance, and pass
 * it back only into setReadCursor() on that SAME instance -- never
 * fabricate one, and never reuse a cursor captured from a different
 * instance.
 *
 * A struct, not a single size_t (changed 2026-09-26, after this buffer
 * gained multi-client support -- see ESP_PicoW_pfodWebServer.cpp's
 * rawDataClients[], where several independently-polling clients each
 * hold their own pfodReadCursor into the SAME buffer instance, at their
 * own separate cadences):
 *   pos               -- absolute (never-wrapping-in-the-caller-visible-
 *                         sense) write-count position, not a raw buffer
 *                         index -- this is what makes setReadCursor()
 *                         able to tell EXACTLY whether a cursor's data
 *                         has since been evicted, rather than guessing
 *                         from wrapped positions alone (an earlier
 *                         version of this API used a raw wrapped index
 *                         and could, in principle, alias a stale cursor
 *                         with a fresh one after exactly one bufferSize
 *                         of untracked traffic -- replaced 2026-09-21).
 *   totalBytesWritten -- a copy of the buffer's own running write
 *                         counter at the exact moment this cursor was
 *                         captured. Carried explicitly, per cursor,
 *                         since an earlier version (2026-09-21 to
 *                         2026-09-26) kept ONLY that counter, privately,
 *                         inside the buffer itself, and periodically
 *                         rebased it down (subtracting bufferSize once
 *                         it grew past double that) to keep it
 *                         "bounded" -- safe for a single owner that
 *                         always refreshes its cursor same-call, but a
 *                         real, reproducible bug once several
 *                         independent clients each hold a cursor across
 *                         separate round trips at different cadences:
 *                         a rebase triggered by one (active) client's
 *                         call silently shifted the numbering every
 *                         OTHER client's still-outstanding, nowhere-
 *                         near-actually-evicted cursor was measured
 *                         against, spuriously invalidating it and
 *                         redelivering already-seen data. Now that the
 *                         counter is never rebased -- see
 *                         setReadCursor()'s own comment -- there is no
 *                         shared epoch left to alias against, so this
 *                         bug class cannot recur; the field is kept
 *                         explicit (rather than folded back into a
 *                         single opaque number) so this invariant --
 *                         nothing may ever again rebase this counter
 *                         out from under an outstanding cursor -- stays
 *                         visible to whoever next reads this class.
 *
 * Contract: call getReadCursor() again immediately after every
 * setReadCursor() so no read progress is lost.
 *
 * `totalBytesWritten` is free to overflow (wrap around 0) over a long
 * enough lifetime -- unsigned overflow is well-defined in C++, not
 * undefined behaviour -- and setReadCursor() handles that correctly
 * (2026-09-26): eviction is decided from `totalBytesWritten_now -
 * cursor.pos`, plain unsigned subtraction, which naturally yields the
 * true elapsed byte count across a SINGLE wraparound without any special
 * casing, the same technique TCP uses for its own wrapping sequence
 * numbers. (This is NOT the periodic, artificial "subtract bufferSize"
 * rebase an earlier, 2026-09-21 to 2026-09-26 version of this class did
 * -- that rebased a SHARED counter far more often than any real overflow
 * risk required, which is what caused the multi-client bug described
 * above. Real wraparound, at the counter's own natural width, is
 * astronomically rarer -- billions of bytes -- and, handled this way,
 * safe for every outstanding cursor without exception.) `valid` exists
 * only so PFOD_CURSOR_START can never be confused with a real captured
 * cursor, including the one-in-2^size_t_bits case where a genuine
 * capture happens to land exactly on the sentinel's own field values.
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
     * Adds a complete line to the buffer (must include its own '\n' or
     * '\r\n' terminator). Uses Print interface to write characters. If
     * wrapping occurs and overwrites start position, adjusts start to
     * next complete line.
     *
     * @param line String containing complete line with '\n' or '\r\n' terminator
     * @return Number of bytes written
     */
    size_t addLine(const char* line);

    /**
     * Adds a complete line to the buffer (must include its own '\n' or
     * '\r\n' terminator). Uses Print interface to write characters. If
     * wrapping occurs and overwrites start position, adjusts start to
     * next complete line.
     *
     * @param line String containing complete line with '\n' or '\r\n' terminator
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
     * Resumes reading from a previously-captured cursor, through to
     * whatever is CURRENTLY the newest data (the buffer's current
     * head) -- "catch me up from where I left off". Safe to call
     * repeatedly, forever, even across long gaps during which
     * eviction may have discarded some of the data that was available
     * when `cursor` was captured -- the comparison is EXACT (an
     * absolute write-count comparison, not a wrapped-position guess),
     * so a cursor whose data has genuinely been evicted is always
     * correctly detected as such, no matter how long the gap.
     *
     * If `cursor` is PFOD_CURSOR_START, or the data at `cursor` has
     * since been evicted, starts instead from the current oldest
     * available byte (same starting point resetForRead() uses) -- so
     * no already-delivered data is skipped, though some of it may be
     * redelivered if eviction happened while nobody was reading.
     *
     * Never artificially rebases totalBytesWritten (unlike an earlier,
     * 2026-09-21 to 2026-09-26 version of this class -- see
     * pfodReadCursor's own doc comment for the real, reproducible
     * multi-client bug that caused). It's left to grow -- and, given
     * enough lifetime, to overflow -- naturally instead; see
     * pfodReadCursor's own doc comment for why that's handled correctly
     * here (plain unsigned wraparound, not undefined behaviour) rather
     * than needing to be avoided.
     *
     * Shares the same underlying read-position state as
     * setReadRange()/resetForRead() -- do not mix cursor-based and
     * range-based reads on the same instance for two different
     * logical read sequences; whichever call happened most recently
     * wins.
     *
     * @param cursor A value previously returned by getReadCursor() on
     *               this SAME instance, or PFOD_CURSOR_START.
     */
    void setReadCursor(pfodReadCursor cursor);

    /**
     * Captures the current read position for passing to
     * setReadCursor() later (typically persisted in a caller-owned
     * variable across many separate calls over time) to resume
     * reading from here onward. Call AFTER reading whatever is
     * currently available.
     *
     * @return Opaque cursor representing the current read position.
     */
    pfodReadCursor getReadCursor() const;

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
     * Automatically wraps and adjusts start position when a line ending
     * ('\n', bare or as the second byte of '\r\n') is detected.
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

    // Absolute (does not wrap every bufferSize bytes the way a raw buffer
    // index would) count of every byte ever written, used ONLY by
    // getReadCursor()/setReadCursor() to give pfodReadCursor an exact,
    // non-aliasing representation -- see pfodReadCursor's own doc comment.
    // Deliberately NEVER artificially rebased (see setReadCursor()'s own
    // comment for why an earlier, periodically-rebased version of this
    // counter was a real multi-client bug) -- left to grow, and eventually
    // wrap at size_t's own natural width, on its own; that wrap is plain
    // unsigned overflow (well-defined, not undefined behaviour) and is
    // handled correctly by setReadCursor()'s wraparound-safe subtraction.
    size_t totalBytesWritten;


    /**
     * Finds the next complete line starting from given position. A line
     * ends on '\n' (bare, or the second byte of a '\r\n' pair) -- see
     * write()'s own comment for the full rule.
     *
     * @param fromPos Starting position to search
     * @param lineEnd Output parameter for position after the '\n'
     * @return true if a complete line was found, false if no '\n' found
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
