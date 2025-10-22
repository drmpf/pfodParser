/*
   messageViewer.js
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */

/**
 * MessageCollector - Centralized collector for raw messages from all connections
 * Stores messages with metadata (timestamp, direction, connection type, size)
 */
class MessageCollector {
  constructor(maxMessages = 500) {
    this.messages = [];
    this.maxMessages = maxMessages;
    this.subscribers = []; // Callback functions to notify of new messages
    this.isPaused = false;
    console.log('[MESSAGE_COLLECTOR] Created with max messages:', maxMessages);
  }

  /**
   * Add a message to the collector
   * @param {string} direction - 'sent' or 'received'
   * @param {string} message - The raw message text
   * @param {string} protocol - 'http', 'serial', or 'ble'
   * @param {string} cmd - Optional command that was sent (for reference)
   */
  addMessage(direction, message, protocol, cmd = null) {
    if (this.isPaused) {
      return;
    }

    const entry = {
      timestamp: new Date().toISOString(),
      direction: direction,
      protocol: protocol,
      message: message,
      cmd: cmd,
      size: message ? message.length : 0
    };

    this.messages.push(entry);

    // Trim to max messages if needed
    if (this.messages.length > this.maxMessages) {
      this.messages.shift();
    }

    // Notify subscribers
    this.notifySubscribers(entry);

    const logPrefix = direction === 'sent' ? '>>> SENT' : '<<< RECEIVED';
    console.log(`[MESSAGE_COLLECTOR] ${logPrefix} [${protocol}] ${message ? message.substring(0, 100) : '(empty)'}`);
  }

  /**
   * Subscribe to new messages
   * @param {function} callback - Function to call with new message entry
   */
  subscribe(callback) {
    this.subscribers.push(callback);
  }

  /**
   * Unsubscribe from messages
   * @param {function} callback - The callback to remove
   */
  unsubscribe(callback) {
    this.subscribers = this.subscribers.filter(cb => cb !== callback);
  }

  /**
   * Notify all subscribers of a new message
   */
  notifySubscribers(entry) {
    this.subscribers.forEach(callback => {
      try {
        callback(entry);
      } catch (error) {
        console.error('[MESSAGE_COLLECTOR] Error in subscriber callback:', error);
      }
    });
  }

  /**
   * Get all messages
   */
  getMessages() {
    return [...this.messages];
  }

  /**
   * Get messages filtered by protocol
   */
  getMessagesByProtocol(protocol) {
    return this.messages.filter(msg => msg.protocol === protocol);
  }

  /**
   * Get messages filtered by direction
   */
  getMessagesByDirection(direction) {
    return this.messages.filter(msg => msg.direction === direction);
  }

  /**
   * Clear all messages
   */
  clear() {
    this.messages = [];
    console.log('[MESSAGE_COLLECTOR] Messages cleared');
  }

  /**
   * Pause collecting messages
   */
  pause() {
    this.isPaused = true;
    console.log('[MESSAGE_COLLECTOR] Paused');
  }

  /**
   * Resume collecting messages
   */
  resume() {
    this.isPaused = false;
    console.log('[MESSAGE_COLLECTOR] Resumed');
  }

  /**
   * Export messages as JSON
   */
  exportAsJSON() {
    return JSON.stringify(this.messages, null, 2);
  }

  /**
   * Export messages as CSV
   */
  exportAsCSV() {
    if (this.messages.length === 0) {
      return 'timestamp,direction,protocol,message,size\n';
    }

    const header = 'timestamp,direction,protocol,message,size\n';
    const rows = this.messages.map(msg => {
      const message = msg.message.replace(/"/g, '""').replace(/\n/g, ' '); // Escape quotes and newlines
      return `"${msg.timestamp}","${msg.direction}","${msg.protocol}","${message}",${msg.size}`;
    });

    return header + rows.join('\n');
  }
}

/**
 * RawMessageViewer - UI component to display collected messages
 */
class RawMessageViewer {
  constructor(messageCollector, containerId = 'raw-message-viewer') {
    this.collector = messageCollector;
    this.containerId = containerId;
    this.isVisible = false;
    this.filterDirection = 'all'; // 'all', 'sent', 'received'
    this.autoScroll = true;
    this.messageViews = []; // Store references to message view elements for scrolling

    console.log('[RAW_MESSAGE_VIEWER] Created with container:', containerId);

    // Subscribe to new messages
    this.collector.subscribe((entry) => this.onNewMessage(entry));
  }

  /**
   * Initialize and create the viewer UI
   */
  initialize() {
    const container = document.getElementById(this.containerId);
    if (!container) {
      console.error('[RAW_MESSAGE_VIEWER] Container not found:', this.containerId);
      return;
    }

    this.createViewerHTML(container);
    this.attachEventListeners();
    this.updateMessageDisplay();
    console.log('[RAW_MESSAGE_VIEWER] Initialized');
  }

  /**
   * Create the HTML structure for the viewer
   */
  createViewerHTML(container) {
    container.innerHTML = `
      <div class="raw-message-viewer" id="raw-message-viewer-main">
        <div class="raw-message-header">
          <div class="raw-message-title">
            <span>Raw Message Viewer</span>
            <button class="raw-message-close-btn" id="raw-message-close-btn" title="Close viewer">&times;</button>
          </div>
          <div class="raw-message-toolbar">
            <div class="raw-message-filters">
              <select id="raw-msg-filter-direction" class="raw-message-filter">
                <option value="all">Direction: All</option>
                <option value="sent">Direction: Sent</option>
                <option value="received">Direction: Received</option>
              </select>
            </div>
            <div class="raw-message-buttons">
              <label class="raw-message-checkbox">
                <input type="checkbox" id="raw-msg-autoscroll" checked>
                Auto-scroll
              </label>
              <button id="raw-msg-clear-btn" class="raw-message-btn">Clear</button>
              <button id="raw-msg-export-json-btn" class="raw-message-btn">Export JSON</button>
              <button id="raw-msg-export-csv-btn" class="raw-message-btn">Export CSV</button>
            </div>
          </div>
        </div>
        <div class="raw-message-content">
          <div class="raw-message-list" id="raw-message-list">
            <div class="raw-message-empty">No messages yet</div>
          </div>
        </div>
      </div>
    `;

    this.attachStyles(container);
  }

  /**
   * Attach CSS styles to the container
   */
  attachStyles(container) {
    const style = document.createElement('style');
    style.textContent = `
      #${this.containerId} {
        all: initial;
        display: none;
        flex-direction: column;
        width: 100%;
        height: 100%;
        font-family: 'Courier New', monospace;
        min-width: 200px;
        background-color: #1e1e1e;
      }

      .raw-message-viewer {
        display: flex;
        flex-direction: column;
        height: 100%;
        width: 100%;
        background-color: #1e1e1e;
        color: #d4d4d4;
        border: 1px solid #333;
        box-sizing: border-box;
        z-index: 5000;
      }

      .raw-message-header {
        flex-shrink: 0;
        background-color: #252526;
        border-bottom: 1px solid #3e3e42;
        padding: 8px;
      }

      .raw-message-title {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-bottom: 8px;
        font-weight: bold;
        font-size: 14px;
        color: #cccccc;
      }

      .raw-message-close-btn {
        background: none;
        border: none;
        color: #cccccc;
        font-size: 20px;
        cursor: pointer;
        padding: 0;
        width: 24px;
        height: 24px;
        display: flex;
        align-items: center;
        justify-content: center;
      }

      .raw-message-close-btn:hover {
        color: #ffffff;
      }

      .raw-message-toolbar {
        display: flex;
        justify-content: space-between;
        align-items: center;
        flex-wrap: wrap;
        gap: 8px;
      }

      .raw-message-filters {
        display: flex;
        gap: 8px;
      }

      .raw-message-filter,
      .raw-message-btn,
      .raw-message-checkbox input {
        background-color: #3c3c3c;
        color: #d4d4d4;
        border: 1px solid #555;
        padding: 4px 8px;
        border-radius: 3px;
        font-size: 12px;
        cursor: pointer;
        font-family: Arial, sans-serif;
      }

      .raw-message-filter:hover,
      .raw-message-btn:hover {
        background-color: #454545;
      }

      .raw-message-filter:focus,
      .raw-message-btn:focus {
        outline: 1px solid #007acc;
      }

      .raw-message-buttons {
        display: flex;
        gap: 8px;
      }

      .raw-message-checkbox {
        display: flex;
        align-items: center;
        gap: 4px;
        cursor: pointer;
        font-size: 12px;
        color: #d4d4d4;
      }

      .raw-message-checkbox input {
        cursor: pointer;
        padding: 0;
        width: 16px;
        height: 16px;
      }

      .raw-message-content {
        flex: 1;
        overflow: hidden;
        display: flex;
        flex-direction: column;
      }

      .raw-message-list {
        flex: 1;
        overflow-y: auto;
        overflow-x: auto;
        background-color: #1e1e1e;
        padding: 4px;
      }

      .raw-message-item {
        display: flex;
        padding: 4px;
        margin: 2px 0;
        border-radius: 2px;
        border-left: 3px solid;
        white-space: pre-wrap;
        word-wrap: break-word;
        font-size: 11px;
        line-height: 1.4;
      }

      .raw-message-item.sent {
        border-left-color: #4ec9b0;
        background-color: #1e3b2a;
      }

      .raw-message-item.received {
        border-left-color: #ce9178;
        background-color: #3b2a1e;
      }

      .raw-message-item-time {
        color: #858585;
        min-width: 100px;
        margin-right: 0px;
        flex-shrink: 0;
      }

      .raw-message-item-direction {
        color: #d7ba7d;
        min-width: 20px;
        margin-right: 2px;
        flex-shrink: 0;
        font-weight: bold;
      }

      .raw-message-item-text {
        flex: 1;
        color: #ce9178;
        word-break: break-all;
      }

      .raw-message-empty {
        color: #858585;
        padding: 20px;
        text-align: center;
        font-style: italic;
      }

      /* Scrollbar styling */
      .raw-message-list::-webkit-scrollbar {
        width: 12px;
        height: 12px;
      }

      .raw-message-list::-webkit-scrollbar-track {
        background-color: #1e1e1e;
      }

      .raw-message-list::-webkit-scrollbar-thumb {
        background-color: #464647;
        border-radius: 4px;
      }

      .raw-message-list::-webkit-scrollbar-thumb:hover {
        background-color: #5a5a5a;
      }
    `;
    document.head.appendChild(style);
  }

  /**
   * Attach event listeners to controls
   */
  attachEventListeners() {
    document.getElementById('raw-message-close-btn')?.addEventListener('click', () => this.hide());
    document.getElementById('raw-msg-filter-direction')?.addEventListener('change', (e) => {
      this.filterDirection = e.target.value;
      this.updateMessageDisplay();
    });
    document.getElementById('raw-msg-autoscroll')?.addEventListener('change', (e) => {
      this.autoScroll = e.target.checked;
    });
    document.getElementById('raw-msg-clear-btn')?.addEventListener('click', () => {
      this.collector.clear();
      this.updateMessageDisplay();
    });
    document.getElementById('raw-msg-export-json-btn')?.addEventListener('click', () => this.exportJSON());
    document.getElementById('raw-msg-export-csv-btn')?.addEventListener('click', () => this.exportCSV());
  }

  /**
   * Called when a new message is added to the collector
   */
  onNewMessage(entry) {
    if (!this.isVisible) {
      return; // Don't update if not visible
    }

    if (!this.shouldDisplayMessage(entry)) {
      return;
    }

    this.addMessageToDisplay(entry);

    if (this.autoScroll) {
      const messageList = document.getElementById('raw-message-list');
      if (messageList) {
        messageList.scrollTop = messageList.scrollHeight;
      }
    }
  }

  /**
   * Check if message should be displayed based on current filters
   */
  shouldDisplayMessage(entry) {
    if (this.filterDirection !== 'all' && entry.direction !== this.filterDirection) {
      return false;
    }
    return true;
  }

  /**
   * Add a message to the display
   */
  addMessageToDisplay(entry) {
    const messageList = document.getElementById('raw-message-list');
    if (!messageList) return;

    // Remove empty message if it exists
    const emptyMsg = messageList.querySelector('.raw-message-empty');
    if (emptyMsg) {
      emptyMsg.remove();
    }

    const messageEl = document.createElement('div');
    messageEl.className = `raw-message-item ${entry.direction}`;

    const timeEl = document.createElement('span');
    timeEl.className = 'raw-message-item-time';
    const time = new Date(entry.timestamp).toLocaleTimeString('en-US', {
      hour12: false,
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit',
      fractionalSecondDigits: 3
    });
    timeEl.textContent = time;

    const directionEl = document.createElement('span');
    directionEl.className = 'raw-message-item-direction';
    directionEl.textContent = entry.direction === 'sent' ? '<<' : '>>';

    const textEl = document.createElement('span');
    textEl.className = 'raw-message-item-text';
    textEl.textContent = entry.message;

    messageEl.appendChild(timeEl);
    messageEl.appendChild(directionEl);
    messageEl.appendChild(textEl);

    messageList.appendChild(messageEl);
    this.messageViews.push(messageEl);

    // Limit number of visible elements to prevent memory issues
    const maxVisibleMessages = 1000;
    if (this.messageViews.length > maxVisibleMessages) {
      const removed = this.messageViews.shift();
      removed.remove();
    }
  }

  /**
   * Update the entire message display based on filters
   */
  updateMessageDisplay() {
    const messageList = document.getElementById('raw-message-list');
    if (!messageList) return;

    messageList.innerHTML = '';
    this.messageViews = [];

    const messages = this.collector.getMessages();
    const filteredMessages = messages.filter(msg => this.shouldDisplayMessage(msg));

    if (filteredMessages.length === 0) {
      messageList.innerHTML = '<div class="raw-message-empty">No messages match the filters</div>';
      return;
    }

    filteredMessages.forEach(entry => this.addMessageToDisplay(entry));

    if (this.autoScroll) {
      messageList.scrollTop = messageList.scrollHeight;
    }
  }

  /**
   * Show the viewer
   */
  show() {
    const container = document.getElementById(this.containerId);
    if (container) {
      container.style.display = 'flex';
      container.style.flexDirection = 'column';
      container.style.flex = 0.30; // 30% width
      this.isVisible = true;
      this.updateMessageDisplay();

      // Show the divider
      const divider = document.getElementById('resize-divider');
      if (divider) {
        divider.style.display = 'block';
      }

      // Set canvas pane to 70%
      const canvasPane = document.getElementById('canvas-pane');
      if (canvasPane) {
        canvasPane.style.flex = 0.70;
      }

      console.log('[RAW_MESSAGE_VIEWER] Shown - canvas 70%, viewer 30%');
    }
  }

  /**
   * Hide the viewer
   */
  hide() {
    const container = document.getElementById(this.containerId);
    if (container) {
      container.style.display = 'none';
      container.style.flex = 0; // Take no space
      this.isVisible = false;

      // Hide the divider
      const divider = document.getElementById('resize-divider');
      if (divider) {
        divider.style.display = 'none';
      }

      // Set canvas pane to 100%
      const canvasPane = document.getElementById('canvas-pane');
      if (canvasPane) {
        canvasPane.style.flex = 1;
      }

      console.log('[RAW_MESSAGE_VIEWER] Hidden - canvas 100%');
    }
  }

  /**
   * Toggle viewer visibility
   */
  toggle() {
    if (this.isVisible) {
      this.hide();
    } else {
      this.show();
    }
  }

  /**
   * Export messages as JSON file
   */
  exportJSON() {
    const json = this.collector.exportAsJSON();
    const blob = new Blob([json], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = `pfod-messages-${new Date().toISOString().replace(/:/g, '-')}.json`;
    link.click();
    URL.revokeObjectURL(url);
    console.log('[RAW_MESSAGE_VIEWER] Exported as JSON');
  }

  /**
   * Export messages as CSV file
   */
  exportCSV() {
    const csv = this.collector.exportAsCSV();
    const blob = new Blob([csv], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = `pfod-messages-${new Date().toISOString().replace(/:/g, '-')}.csv`;
    link.click();
    URL.revokeObjectURL(url);
    console.log('[RAW_MESSAGE_VIEWER] Exported as CSV');
  }
}

// Make classes available globally
window.MessageCollector = MessageCollector;
window.RawMessageViewer = RawMessageViewer;
