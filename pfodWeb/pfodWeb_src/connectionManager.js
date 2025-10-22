/*
   connectionManager.js
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */

/**
 * Custom alert modal with pfodWeb branding
 * Shows a styled modal dialog positioned lower on the page
 * @param {string} message - The message to display
 * @param {function} onClose - Optional callback when Close button is clicked
 */
function pfodAlert(message, onClose = null) {
  // Create modal overlay
  const overlay = document.createElement('div');
  overlay.style.cssText = `
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    background-color: rgba(0, 0, 0, 0.5);
    display: flex;
    align-items: flex-start;
    justify-content: center;
    padding-top: 150px;
    z-index: 10000;
  `;

  // Create modal box
  const modal = document.createElement('div');
  modal.style.cssText = `
    background-color: white;
    border-radius: 10px;
    box-shadow: 0 4px 20px rgba(0, 0, 0, 0.3);
    max-width: 500px;
    width: 90%;
    overflow: hidden;
  `;

  // Create title bar
  const titleBar = document.createElement('div');
  titleBar.style.cssText = `
    background-color: #4CAF50;
    color: white;
    padding: 15px 20px;
    font-size: 18px;
    font-weight: bold;
    font-family: Arial, sans-serif;
  `;
  titleBar.textContent = 'pfodWeb';

  // Create message area
  const messageArea = document.createElement('div');
  messageArea.style.cssText = `
    padding: 20px;
    font-family: Arial, sans-serif;
    font-size: 14px;
    line-height: 1.6;
    color: #333;
    white-space: pre-line;
  `;
  messageArea.textContent = message;

  // Assemble modal
  modal.appendChild(titleBar);
  modal.appendChild(messageArea);

  // Add Close button if callback provided
  if (onClose) {
    const buttonArea = document.createElement('div');
    buttonArea.style.cssText = `
      padding: 0 20px 20px 20px;
      text-align: center;
    `;

    const closeButton = document.createElement('button');
    closeButton.textContent = 'Close';
    closeButton.style.cssText = `
      background-color: #4CAF50;
      color: white;
      padding: 10px 30px;
      border: none;
      border-radius: 5px;
      font-size: 16px;
      font-weight: bold;
      cursor: pointer;
      font-family: Arial, sans-serif;
    `;
    const closeAction = () => {
      document.body.removeChild(overlay);
      onClose();
    };
    closeButton.onclick = closeAction;

    // Add Enter key handler for the overlay
    overlay.addEventListener('keydown', (event) => {
      if (event.key === 'Enter') {
        event.preventDefault();
        closeAction();
      }
    });

    buttonArea.appendChild(closeButton);
    modal.appendChild(buttonArea);

    // Focus the close button so Enter key works immediately
    setTimeout(() => closeButton.focus(), 100);
  }

  overlay.appendChild(modal);

  // Add to page
  document.body.appendChild(overlay);
}

/**
 * Shared dedup mechanism - used by all connection protocols
 * Rotating character prepended to commands to detect duplicates
 * Only increments on successful sends, not on retries
 */
let dedupCounter = 0;
const dedupChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

function getCurrentDedupChar() {
  return dedupChars[dedupCounter];
}

function advanceDedupChar() {
  dedupCounter = (dedupCounter + 1) % dedupChars.length;
}

/**
 * Find the index of the closing brace that matches the opening brace at startIdx
 * Handles nested braces by counting depth
 * @param {string} text - The text to search
 * @param {number} startIdx - Index of opening brace
 * @returns {number} Index of matching closing brace, or -1 if not found
 */
function findMatchingClosingBrace(text, startIdx) {
  if (startIdx < 0 || startIdx >= text.length || text[startIdx] !== '{') {
    return -1;
  }

  let depth = 0;
  for (let i = startIdx; i < text.length; i++) {
    if (text[i] === '{') {
      depth++;
    } else if (text[i] === '}') {
      depth--;
      if (depth === 0) {
        return i;
      }
    }
  }
  return -1; // No matching closing brace found
}

/**
 * ConnectionManager - Unified connection abstraction for multiple protocols
 *
 * Provides a protocol-agnostic interface for communicating with pfod devices.
 * Currently supports: HTTP, Serial, BLE
 */
class ConnectionManager {
  // Static message collector shared across all connection managers
  static messageCollector = null;

  static setMessageCollector(collector) {
    ConnectionManager.messageCollector = collector;
    console.log('[CONNECTION_MANAGER] Message collector set');
  }

  constructor(config = {}) {
    this.protocol = config.protocol || 'http';
    this.adapter = null;
    this.config = config;

    // Response timeout configuration
    // Default is 3 seconds, configurable from 0 (never, actually 60sec) to 30 seconds
    // Timeout value in seconds from config, default to 3
    const timeoutSeconds = config.responseTimeoutSec !== undefined ? config.responseTimeoutSec : 3;

    // Validate and constrain timeout: 0-30 seconds
    let validatedTimeout = Math.max(0, Math.min(30, timeoutSeconds));

    // Convert to milliseconds, with special case: 0 means "never" (use 60 seconds)
    if (validatedTimeout === 0) {
      this.responseTimeoutMs = 60000;  // 60 seconds for "never"
      console.log(`[CONNECTION_MANAGER] Response timeout set to 0 (never) - using 60 seconds actual timeout`);
    } else {
      this.responseTimeoutMs = validatedTimeout * 1000;
      console.log(`[CONNECTION_MANAGER] Response timeout set to ${validatedTimeout} seconds`);
    }

    // Set up max retries based on protocol
    // BLE = 0 (unreliable, fail fast)
    // HTTP = 0 (fail fast for network)
    // Serial = 1 (most reliable, allow one retry)
    const retryConfig = {
      'ble': 0,
      'http': 2,
      'serial': 2
    };
    // Use explicit check for protocol key to handle 0 values correctly, default to 0
    this.maxRetries = (this.protocol in retryConfig) ? retryConfig[this.protocol] : 0;
    console.log(`[CONNECTION_MANAGER] Max retries set to ${this.maxRetries} for protocol: ${this.protocol}`);

    console.log(`[CONNECTION_MANAGER] Creating connection manager with protocol: ${this.protocol}`);

    // Initialize the appropriate protocol adapter
    this.initializeAdapter();
  }

  initializeAdapter() {
    switch(this.protocol) {
      case 'http':
        this.adapter = new HTTPConnection(this.config, this);
        console.log(`[CONNECTION_MANAGER] Initialized HTTP adapter with targetIP: ${this.config.targetIP}`);
        break;

      case 'serial':
        this.adapter = new SerialConnection(this.config, this);
        console.log(`[CONNECTION_MANAGER] Initialized Serial adapter`);
        break;

      case 'ble':
        this.adapter = new BLEConnection(this.config, this);
        console.log(`[CONNECTION_MANAGER] Initialized BLE adapter`);
        break;

      default:
        throw new Error(`Unknown protocol: ${this.protocol}`);
    }
  }

  /**
   * Send a command to the device and get response
   * @param {string} cmd - The pfod command (e.g., "{.}" or "{dwgName}")
   * @returns {Promise<string>} - Response text (usually JSON)
   */
  async send(cmd) {
    if (!this.adapter) {
      throw new Error('No adapter initialized');
    }

    console.log(`[CONNECTION_MANAGER] Sending command: ${cmd}`);

    const response = await this.adapter.send(cmd);
    console.log(`[CONNECTION_MANAGER] Received response (${response.length} bytes)`);

    return response;
  }

  /**
   * Connect to the device (if needed for the protocol)
   */
  async connect() {
    if (this.adapter && this.adapter.connect) {
      console.log(`[CONNECTION_MANAGER] Connecting via ${this.protocol}...`);
      await this.adapter.connect();
      console.log(`[CONNECTION_MANAGER] Connected`);
    }
  }

  /**
   * Disconnect from the device (if needed for the protocol)
   */
  async disconnect() {
    if (this.adapter && this.adapter.disconnect) {
      console.log(`[CONNECTION_MANAGER] Disconnecting...`);
      await this.adapter.disconnect();
      console.log(`[CONNECTION_MANAGER] Disconnected`);
    }
  }

  /**
   * Check if connection is active
   */
  isConnected() {
    if (this.adapter && this.adapter.isConnected) {
      return this.adapter.isConnected();
    }
    return true; // HTTP doesn't need explicit connection
  }

  /**
   * Get response timeout in milliseconds for waiting for device response
   * Returns the configured timeout value
   * @returns {number} - Timeout in milliseconds
   */
  getResponseTimeout() {
    return this.responseTimeoutMs;
  }

  /**
   * Get max retries for the current connection protocol
   * @returns {number} - Max retries (BLE: 1, HTTP: 2, Serial: 3)
   */
  getMaxRetries() {
    return this.maxRetries;
  }
}

/**
 * HTTPConnection - Adapter for HTTP protocol
 *
 * Handles communication with pfod devices over HTTP.
 * Supports CORS for cross-origin requests.
 */
class HTTPConnection {
  constructor(config, connectionManager) {
    this.config = config;
    this.connectionManager = connectionManager;
    this.targetIP = config.targetIP;
    this.baseURL = this.targetIP ? `http://${this.targetIP}` : '';
    this.timeoutId = null;  // Store timeout ID so it can be cancelled

    console.log(`[HTTP_CONNECTION] Created with baseURL: ${this.baseURL || '(relative)'}`);
  }

  /**
   * Build fetch options with appropriate CORS settings
   * @returns {object} - Fetch options
   */
  buildFetchOptions() {
    return {
      headers: {
        'Accept': 'application/json',
        'X-Requested-With': 'XMLHttpRequest'
      },
      mode: this.targetIP ? 'cors' : 'same-origin',
      credentials: this.targetIP ? 'omit' : 'same-origin',
      cache: 'no-cache'
    };
  }

  /**
   * Send a command via HTTP and return the response text
   * @param {string} cmd - The pfod command (e.g., "{.}" or "{dwgName}")
   * @returns {Promise<string>} - Response text
   */
  async send(cmd) {
    // Prepend current dedup character to the command (don't advance yet - only advance on success)
    const cmdWithPrefix = getCurrentDedupChar() + cmd;

    // Build endpoint from command with prefix
    const endpoint = this.baseURL + `/pfodWeb?cmd=${encodeURIComponent(cmdWithPrefix)}`;
    const options = this.buildFetchOptions();

    // Cancel any previous timeout that might still be running
    if (this.timeoutId) {
      clearTimeout(this.timeoutId);
      console.log(`[HTTP_CONNECTION] Cancelled previous timeout`);
      this.timeoutId = null;
    }

    // Add timeout using AbortController
    const controller = new AbortController();
    const timeout = this.connectionManager.getResponseTimeout();
    console.log(`[HTTP_CONNECTION] Setting response timeout to ${timeout}ms`);
    this.timeoutId = setTimeout(() => controller.abort(), timeout);

    try {
      console.log(`[HTTP_CONNECTION] Fetching: ${endpoint}`);

      // Record the command being sent (with the dedup prefix)
      if (ConnectionManager.messageCollector) {
        ConnectionManager.messageCollector.addMessage('sent', cmdWithPrefix, 'http', cmd);
      }

      const response = await fetch(endpoint, {
        ...options,
        signal: controller.signal
      });

      console.log(`[HTTP_CONNECTION] Response status: ${response.status}`);

      if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }

      const responseText = await response.text();

      // Clear timeout on successful response
      if (this.timeoutId) {
        clearTimeout(this.timeoutId);
        this.timeoutId = null;
      }

      // Record the response in the message collector using byte-by-byte scan
      if (ConnectionManager.messageCollector) {
        // Find text before {
        const startBrace = responseText.indexOf('{');
        if (startBrace > 0) {
          const beforeText = responseText.substring(0, startBrace);
          if (beforeText.trim()) {
            ConnectionManager.messageCollector.addMessage('received', beforeText, 'http', cmd);
          }
        }

        // Find and record { to } - use matching brace for nested structures
        const endBrace = startBrace >= 0 ? findMatchingClosingBrace(responseText, startBrace) : -1;
        if (startBrace !== -1 && endBrace !== -1) {
          const pfodMessage = responseText.substring(startBrace, endBrace + 1);
          ConnectionManager.messageCollector.addMessage('received', pfodMessage, 'http', cmd);

          // Find text after }
          const afterText = responseText.substring(endBrace + 1);
          if (afterText.trim()) {
            ConnectionManager.messageCollector.addMessage('received', afterText, 'http', cmd);
          }
        } else if (startBrace === -1) {
          // No { found - record entire response as junk
          if (responseText.trim()) {
            ConnectionManager.messageCollector.addMessage('received', responseText, 'http', cmd);
          }
        }
      }

      // Advance dedup character only on successful response (not on retry/timeout)
      advanceDedupChar();

      return responseText;
    } catch (error) {
      // Clear timeout on error
      if (this.timeoutId) {
        clearTimeout(this.timeoutId);
        this.timeoutId = null;
      }

      if (error.name === 'AbortError') {
        throw new Error('HTTP response timeout - device may not be responding');
      }
      throw error;
    }
  }

  /**
   * HTTP doesn't need explicit connection
   */
  async connect() {
    // No-op for HTTP
  }

  /**
   * HTTP doesn't need explicit disconnection
   */
  async disconnect() {
    // No-op for HTTP
  }

  /**
   * HTTP is always "connected"
   */
  isConnected() {
    return true;
  }
}

/**
 * SerialConnection - Adapter for Serial protocol using Web Serial API
 *
 * Handles communication with pfod devices over serial ports (USB, UART, etc).
 * Uses the browser's Web Serial API for direct serial communication.
 */
class SerialConnection {
  constructor(config, connectionManager) {
    this.config = config;
    this.connectionManager = connectionManager;
    this.port = null;
    this.portName = 'Unknown Port';  // Store human-readable port name for error messages
    this.reader = null;
    this.writer = null;
    this.connected = false;
    this.readBuffer = '';
    this.responsePromise = null;
    this.responseResolve = null;
    this.responseReject = null;
    this.timeoutId = null;  // Store timeout ID so it can be cancelled
    this.firstRequest = true;  // Flag to track if this is the first request
    this.firstRequestAttemptTimeout = 2000;  // Start at 1 second for first request attempts

    // Serial configuration with defaults
    this.baudRate = config.baudRate || 9600;
    this.dataBits = config.dataBits || 8;
    this.stopBits = config.stopBits || 1;
    this.parity = config.parity || 'none';
    this.flowControl = 'none'; //'hardware' ;//config.flowControl || 'none';

    console.log(`[SERIAL_CONNECTION] Created with baud rate: ${this.baudRate}`);
  }

  /**
   * Connect to a serial port
   * Always prompts user to select port (does not reuse previously granted ports)
   */
  async connect() {
    try {
      // Check if Web Serial API is supported
      if (!('serial' in navigator)) {
        const errorMsg = 'Web Serial API is not supported in this browser.\n\n' +
                        'Serial connections require:\n' +
                        '• Chrome (version 89 or later)\n' +
                        '• Edge (version 89 or later)\n' +
                        '• Opera (version 75 or later)\n\n' +
                        'Please use a supported browser for Serial connections.';
        throw new Error(errorMsg);
      }

      // Always prompt user to select serial port
      console.log('[SERIAL_CONNECTION] Prompting user to select serial port...');

      try {
        // Request port from user
        this.port = await navigator.serial.requestPort();

        // Capture port name from port info
        try {
          const portInfo = this.port.getInfo();
          let foundComPort = null;

          // Try to find matching port from navigator.serial.getPorts()
          // This works better on Windows where COM port info isn't exposed directly
          try {
            const allPorts = await navigator.serial.getPorts();
            console.log('[SERIAL_CONNECTION] Total available ports:', allPorts.length);

            // Try to find the just-selected port by matching VID/PID
            for (let availablePort of allPorts) {
              const availableInfo = availablePort.getInfo();
              if (availableInfo.usbVendorId === portInfo.usbVendorId &&
                  availableInfo.usbProductId === portInfo.usbProductId) {
                console.log('[SERIAL_CONNECTION] Matched port by VID/PID');

                // Try to extract path/name from available port
                if (availablePort.path) {
                  foundComPort = availablePort.path;
                  console.log('[SERIAL_CONNECTION] Found path:', availablePort.path);
                  break;
                }
              }
            }
          } catch (e) {
            console.warn('[SERIAL_CONNECTION] Error getting ports list:', e);
          }

          // Fallback approaches if above didn't work
          if (!foundComPort) {
            // Try port.path
            if (this.port.path) {
              foundComPort = this.port.path;
              console.log('[SERIAL_CONNECTION] Using port.path:', foundComPort);
            }
          }

          // Set final port name
          if (foundComPort) {
            // If it looks like a COM port, use it as is
            if (foundComPort.match(/COM\d+/)) {
              this.portName = foundComPort.match(/COM\d+/)[0];
            } else {
              this.portName = foundComPort;
            }
          } else {
            // Chrome on Windows doesn't expose COM port number, just show COM?
            this.portName = 'COM?';
          }

          console.log('[SERIAL_CONNECTION] Final port name:', this.portName);
        } catch (e) {
          this.portName = 'Serial Port';
          console.warn('[SERIAL_CONNECTION] Error extracting port name:', e);
        }
        console.log('[SERIAL_CONNECTION] Attempting to open port...');

        // Try to open the newly selected port
        await this.port.open({
          baudRate: this.baudRate,
          dataBits: this.dataBits,
          stopBits: this.stopBits,
          parity: this.parity,
          flowControl: this.flowControl
        });
        console.log('[SERIAL_CONNECTION] Port opened successfully');
      } catch (selectError) {
        console.error('[SERIAL_CONNECTION] Port selection or opening failed:', selectError);

        const errorMsg = 'Serial port could not be opened. Please ensure:\n' +
                        '1. The device is connected\n' +
                        '2. No other application is using the port\n' +
                        '3. You selected the correct port';
        throw new Error(errorMsg);
      }

      // Get reader and writer
      this.reader = this.port.readable.getReader();
      this.writer = this.port.writable.getWriter();
      this.connected = true;

      // Reset first request flag for new connection
      this.firstRequest = true;
      console.log('[SERIAL_CONNECTION] Serial connection established successfully, firstRequest flag reset');

      // Start reading loop (don't await - let it run in background)
      this.startReading();

      // Give a moment for the read loop to actually start before returning
      // This ensures the reader is actively listening before we send commands
      await new Promise(resolve => setTimeout(resolve, 50));

    } catch (error) {
      console.error('[SERIAL_CONNECTION] Connection failed:', error);
      throw error;
    }
  }

  /**
   * Start continuous reading from serial port
   * Buffers incoming data until a complete response is received
   * Resets timeout each time data is received
   */
  async startReading() {
    console.log('[SERIAL_CONNECTION] Starting read loop...');

    try {
      while (this.connected && this.reader) {
        const { value, done } = await this.reader.read();

        if (done) {
          console.log('[SERIAL_CONNECTION] Reader closed');
          break;
        }

        // Convert Uint8Array to string
        const text = new TextDecoder().decode(value);
        this.readBuffer += text;

        // Reset timeout each time we receive data
        if (this.timeoutId && this.responseReject) {
          clearTimeout(this.timeoutId);
          // Use the current timeout value that was set in send()
    //      console.log(`[SERIAL_CONNECTION] Resetting timeout to ${this.currentTimeout}ms after receiving data`);
          this.timeoutId = setTimeout(() => {
            if (this.responseReject) {
              this.responseReject(new Error('Serial response timeout - device may not be responding'));
              this.responseResolve = null;
              this.responseReject = null;
              this.timeoutId = null;
            }
          }, this.currentTimeout);
        }

        // Check if we have a complete pfod response
        this.processReadBuffer();
      }
    } catch (error) {
      if (this.connected) {
        console.error('[SERIAL_CONNECTION] Read error:', error);
        if (this.responseReject) {
          this.responseReject(error);
          this.responseResolve = null;
          this.responseReject = null;
        }
      }
    }
  }

  /**
   * Convert pfod protocol string to JSON format expected by pfodWebDebug
   * pfod format: {,~`0~V2|+A~z}
   * Split by | and } where each starts a new array element
   * Result: {"cmd": ["{,~`0~V2", "|+A~z", "}"]}
   */
  pfodToJson(pfodString) {
    console.log(`[PFOD_TO_JSON] INPUT pfod string:`, pfodString);

    // Split the string where | or } starts a new element
    const cmdArray = [];
    let currentElement = '';

    for (let i = 0; i < pfodString.length; i++) {
      const char = pfodString[i];

      if (char === '|' || char === '}') {
        // Save current element if not empty
        if (currentElement.length > 0) {
          cmdArray.push(currentElement);
        }
        // Start new element with the delimiter
        currentElement = char;

        // If it's }, add it as its own element and reset
        if (char === '}') {
          cmdArray.push(currentElement);
          currentElement = '';
        }
      } else {
        currentElement += char;
      }
    }

    // Add any remaining element (shouldn't happen with well-formed pfod)
    if (currentElement.length > 0) {
      cmdArray.push(currentElement);
    }

    // Wrap in JSON structure
    const jsonObject = {
      cmd: cmdArray
    };

    const jsonString = JSON.stringify(jsonObject);
    console.log(`[PFOD_TO_JSON] OUTPUT JSON:`, jsonString);

    return jsonString;
  }

  /**
   * Process the read buffer for complete pfod responses
   * Parsing starts on { and stops on }
   * Captures ALL text including chars outside { }
   */
  processReadBuffer() {
    // Find the start of a pfod command
    const startBrace = this.readBuffer.indexOf('{');

    if (startBrace === -1) {
      // No start brace yet - but capture any non-empty text as raw output
      if (this.readBuffer.length > 0) {
        if (ConnectionManager.messageCollector) {
          ConnectionManager.messageCollector.addMessage('received', this.readBuffer, 'serial');
        }
      }
      this.readBuffer = '';
      return;
    }

    // Capture any text BEFORE the start brace
    if (startBrace > 0) {
      const beforeText = this.readBuffer.substring(0, startBrace);
      if (ConnectionManager.messageCollector) {
        ConnectionManager.messageCollector.addMessage('received', beforeText, 'serial');
      }
      this.readBuffer = this.readBuffer.substring(startBrace);
    }

    // Now check if we have the closing brace
    const endBrace = this.readBuffer.indexOf('}');

    if (endBrace === -1) {
      // No end brace yet - keep buffering
      return;
    }

    // We have a complete pfod command from { to }
    const pfodString = this.readBuffer.substring(0, endBrace + 1);

    // Calculate time elapsed since send
    const receiveTime = Date.now();
    const elapsedMs = this.sendTime ? (receiveTime - this.sendTime) : 0;

    console.log(`[SERIAL_CONNECTION] Received complete pfod command after ${elapsedMs}ms:`, pfodString);

    // Convert pfod protocol to JSON format
    const jsonString = this.pfodToJson(pfodString);

    // Record the raw pfod message received
    if (ConnectionManager.messageCollector) {
      ConnectionManager.messageCollector.addMessage('received', pfodString, 'serial');
    }

    if (this.responseResolve) {
      // Cancel the timeout since we got a response
      if (this.timeoutId) {
        clearTimeout(this.timeoutId);
        this.timeoutId = null;
      }

      // Advance dedup character only on successful response (not on retry/timeout)
      advanceDedupChar();

      this.responseResolve(jsonString);
      this.responseResolve = null;
      this.responseReject = null;
    }

    // Clear the buffer up to and including the closing brace
    this.readBuffer = this.readBuffer.substring(endBrace + 1);
  }

  /**
   * Send a command via serial and return the response
   * For first request: starts with 1 second timeout, doubles on each timeout up to user-set timeout
   * Logs warnings (not errors) for first request timeouts during auto-detection
   * @param {string} cmd - The pfod command (e.g., "{.}" or "{dwgName}")
   * @returns {Promise<string>} - Response text (usually JSON)
   */
  async send(cmd) {
    // Auto-connect if not already connected
    if (!this.connected || !this.writer) {
      console.log('[SERIAL_CONNECTION] Not connected, connecting now...');
      await this.connect();
    }

    // Diagnostic logging: Check if a previous request is still pending
    if (this.responseResolve || this.responseReject) {
      console.warn(`[SERIAL_CONNECTION] WARNING: send() called while previous request still pending`);
      console.warn(`[SERIAL_CONNECTION] This should not happen - queue protection may not be working`);
    }

    // Cancel any previous timeout that might still be running
    if (this.timeoutId) {
      clearTimeout(this.timeoutId);
      console.log(`[SERIAL_CONNECTION] Cancelled previous timeout`);
      this.timeoutId = null;
    }

    // Clear previous response state
    this.readBuffer = '';

    // For first request, implement progressive timeout doubling
    if (this.firstRequest) {
      this.firstRequest = false;
      return this.sendWithProgressiveTimeout(cmd);
    }

    // Normal send for subsequent requests
    return this.sendOnce(cmd, this.connectionManager.getResponseTimeout());
  }

  /**
   * Send with progressive timeout doubling for first request
   * EVERY timeout throws error back to caller (pfodWebDebug queue) for retry
   * Timeout doubles on each retry until user-set max or first response received
   */
  async sendWithProgressiveTimeout(cmd) {
    const maxTimeout = this.connectionManager.getResponseTimeout();
    const timeoutToUse = Math.min(this.firstRequestAttemptTimeout, maxTimeout);
    const isMaxTimeout = (timeoutToUse >= maxTimeout);

    console.error(`[SERIAL_CONNECTION] First request attempt with ${timeoutToUse}ms timeout`);

    try {
      // Try to send with current timeout
      const response = await this.sendOnce(cmd, timeoutToUse);

      // Success! Clear first request flag and return
      this.firstRequest = false;
      console.error(`[SERIAL_CONNECTION] First request succeeded with ${timeoutToUse}ms timeout`);
      return response;
    } catch (error) {
      // Check if it was a timeout error
      if (error.message.includes('timeout')) {
        // Double timeout for next retry (unless already at max)
        if (!isMaxTimeout) {
        //  this.firstRequestAttemptTimeout *= 2;
        }

        // Log and throw error back to queue for retry
        if (isMaxTimeout) {
          console.error(`[SERIAL_CONNECTION] First request timeout at ${maxTimeout}ms (max) - will retry`);
          this.firstRequest = false;  // Clear flag when max reached
        } else {
          console.error(`[SERIAL_CONNECTION] First request timeout at ${timeoutToUse}ms - next attempt will use ${this.connectionManager.getResponseTimeout()}ms`);
        }

        // Throw error for EVERY timeout - let queue handle retry
        throw new Error('Serial response timeout - device may not be responding');
      } else {
        // Non-timeout error, propagate immediately
        this.firstRequest = false;  // Clear flag on error
        throw error;
      }
    }
  }

  /**
   * Send command once with specified timeout
   * @param {string} cmd - The pfod command
   * @param {number} timeout - Timeout in milliseconds
   * @returns {Promise<string>} - Response text
   */
  async sendOnce(cmd, timeout) {
    // Record send time for performance measurement
    this.sendTime = Date.now();
    this.currentTimeout = timeout;

    // Set up promise for response
    const responsePromise = new Promise((resolve, reject) => {
      this.responseResolve = resolve;
      this.responseReject = reject;

      console.log(`[SERIAL_CONNECTION] Setting timeout to ${timeout}ms`);
      this.timeoutId = setTimeout(() => {
        if (this.responseReject) {
          this.responseReject(new Error('Serial response timeout - device may not be responding'));
          this.responseResolve = null;
          this.responseReject = null;
          this.timeoutId = null;
        }
      }, timeout);
    });

    // Prepend current dedup character to the command (don't advance yet - only advance on success)
    const cmdWithPrefix = getCurrentDedupChar() + cmd;

    // Send the command
    console.log(`[SERIAL_CONNECTION] Sending: ${cmdWithPrefix} at ${new Date(this.sendTime).toISOString()}`);

    // Record the command being sent (with the dedup prefix)
    if (ConnectionManager.messageCollector) {
      ConnectionManager.messageCollector.addMessage('sent', cmdWithPrefix, 'serial', cmd);
    }

    const encoder = new TextEncoder();
    const data = encoder.encode(cmdWithPrefix + '\n'); // Add newline for command termination
    await this.writer.write(data);

    // Wait for response
    return responsePromise;
  }

  /**
   * Disconnect from the serial port
   */
  async disconnect() {
    console.log('[SERIAL_CONNECTION] Disconnecting...');

    this.connected = false;

    try {
      // Cancel reader
      if (this.reader) {
        await this.reader.cancel();
        this.reader.releaseLock();
        this.reader = null;
      }

      // Release writer
      if (this.writer) {
        this.writer.releaseLock();
        this.writer = null;
      }

      // Close port
      if (this.port) {
        await this.port.close();
        this.port = null;
      }

      console.log('[SERIAL_CONNECTION] Disconnected successfully');
    } catch (error) {
      console.error('[SERIAL_CONNECTION] Error during disconnect:', error);
      throw error;
    }
  }

  /**
   * Check if serial connection is active
   */
  isConnected() {
    return this.connected && this.port !== null;
  }
}

/**
 * BLEConnection - Adapter for BLE protocol using Web Bluetooth API
 *
 * Handles communication with pfod devices over Bluetooth Low Energy.
 * Uses the browser's Web Bluetooth API for direct BLE communication.
 * Filters devices to only show those advertising the UART service.
 */
class BLEConnection {
  constructor(config, connectionManager) {
    this.config = config;
    this.connectionManager = connectionManager;
    this.device = null;
    this.server = null;
    this.service = null;
    this.characteristicTX = null;
    this.characteristicRX = null;
    this.connected = false;
    this.readBuffer = '';
    this.responseResolve = null;
    this.responseReject = null;
    this.timeoutId = null;  // Store timeout ID so it can be cancelled

    // UART Service UUIDs (Nordic UART Service)
    this.UART_SERVICE_UUID = '6E400001-B5A3-F393-E0A9-E50E24DCCA9E'.toLowerCase();
    this.UART_TX_CHAR_UUID = '6E400002-B5A3-F393-E0A9-E50E24DCCA9E'.toLowerCase();
    this.UART_RX_CHAR_UUID = '6E400003-B5A3-F393-E0A9-E50E24DCCA9E'.toLowerCase();

    console.log(`[BLE_CONNECTION] Created with UART service filtering`);
  }

  /**
   * Connect to a BLE device
   * Uses previously granted device if available, otherwise prompts user with filtering
   */
  async connect() {
    try {
      // Check if Web Bluetooth API is supported
      if (!('bluetooth' in navigator)) {
        const errorMsg = 'Web Bluetooth API is not supported in this browser.\n\n' +
                        'Bluetooth connections require:\n' +
                        '• Chrome (version 56 or later)\n' +
                        '• Edge (version 79 or later)\n' +
                        '• Opera (version 43 or later)\n\n' +
                        'Please use a supported browser for Bluetooth connections.';
        throw new Error(errorMsg);
      }

      // Prompt user to select BLE device
      console.log('[BLE_CONNECTION] Prompting user to select BLE device...');

      try {
        // Request device from user with UART service filter
        this.device = await navigator.bluetooth.requestDevice({
          filters: [{services: [this.UART_SERVICE_UUID]}]
        });
        console.log(`[BLE_CONNECTION] User selected device: ${this.device.name || 'Unknown Device'}`);

        // Try to connect to the newly selected device
        await this.connectToDevice(this.device);
      } catch (selectError) {
        console.error('[BLE_CONNECTION] Device selection or connection failed:', selectError);

        const errorMsg = 'BLE device could not be connected. Please ensure:\n' +
                        '1. The device is powered on\n' +
                        '2. The device is within range\n' +
                        '3. The device is advertising the UART service';
        throw new Error(errorMsg);
      }

      console.log('[BLE_CONNECTION] BLE connection established successfully');

    } catch (error) {
      console.error('[BLE_CONNECTION] Connection failed:', error);
      throw error;
    }
  }

  /**
   * Connect to a specific BLE device and set up characteristics
   */
  async connectToDevice(device) {
    console.log(`[BLE_CONNECTION] Connecting to device: ${device.name || 'Unknown Device'}`);

    // Set up disconnect listener
    device.addEventListener('gattserverdisconnected', () => this.onDisconnected());

    // Connect to GATT server
    this.server = await device.gatt.connect();
    console.log('[BLE_CONNECTION] Connected to GATT Server');

    // Get UART service
    this.service = await this.server.getPrimaryService(this.UART_SERVICE_UUID);
    console.log('[BLE_CONNECTION] UART Service discovered');

    // Get RX characteristic (device transmits, we receive)
    this.characteristicRX = await this.service.getCharacteristic(this.UART_RX_CHAR_UUID);
    console.log('[BLE_CONNECTION] RX Characteristic discovered');

    // Set up notification handler
    this.characteristicRX.addEventListener('characteristicvaluechanged', (event) => {
      this.handleCharacteristicChange(event);
    });

    // Start notifications
    await this.characteristicRX.startNotifications();
    console.log('[BLE_CONNECTION] Notifications started');

    // Get TX characteristic (we transmit, device receives)
    this.characteristicTX = await this.service.getCharacteristic(this.UART_TX_CHAR_UUID);
    console.log('[BLE_CONNECTION] TX Characteristic discovered');

    this.connected = true;
    this.device = device;
  }

  /**
   * Handle disconnect event
   */
  onDisconnected() {
    console.log('[BLE_CONNECTION] Device disconnected');
    this.connected = false;
    this.server = null;
    this.service = null;
    this.characteristicTX = null;
    this.characteristicRX = null;
  }

  /**
   * Handle incoming data from BLE device
   * Resets timeout each time data is received
   */
  handleCharacteristicChange(event) {
    const text = new TextDecoder().decode(event.target.value);
    this.readBuffer += text;
    console.log(`[BLE_CONNECTION] Received data: ${text}`);

    // Reset timeout each time we receive data
    if (this.timeoutId && this.responseReject) {
      clearTimeout(this.timeoutId);
      const timeout = this.connectionManager.getResponseTimeout();
      console.log(`[BLE_CONNECTION] Resetting timeout to ${timeout}ms after receiving data`);
      this.timeoutId = setTimeout(() => {
        if (this.responseReject) {
          this.responseReject(new Error('BLE response timeout - device may not be responding'));
          this.responseResolve = null;
          this.responseReject = null;
          this.timeoutId = null;
        }
      }, timeout);
    }

    // Check if we have a complete pfod response
    this.processReadBuffer();
  }

  /**
   * Convert pfod protocol string to JSON format expected by pfodWebDebug
   * Same logic as SerialConnection
   */
  pfodToJson(pfodString) {
    console.log(`[PFOD_TO_JSON] INPUT pfod string:`, pfodString);

    // Split the string where | or } starts a new element
    const cmdArray = [];
    let currentElement = '';

    for (let i = 0; i < pfodString.length; i++) {
      const char = pfodString[i];

      if (char === '|' || char === '}') {
        // Save current element if not empty
        if (currentElement.length > 0) {
          cmdArray.push(currentElement);
        }
        // Start new element with the delimiter
        currentElement = char;

        // If it's }, add it as its own element and reset
        if (char === '}') {
          cmdArray.push(currentElement);
          currentElement = '';
        }
      } else {
        currentElement += char;
      }
    }

    // Add any remaining element (shouldn't happen with well-formed pfod)
    if (currentElement.length > 0) {
      cmdArray.push(currentElement);
    }

    // Wrap in JSON structure
    const jsonObject = {
      cmd: cmdArray
    };

    const jsonString = JSON.stringify(jsonObject);
    console.log(`[PFOD_TO_JSON] OUTPUT JSON:`, jsonString);

    return jsonString;
  }

  /**
   * Process the read buffer for complete pfod responses
   * Same logic as SerialConnection
   */
  processReadBuffer() {
    // Find the start of a pfod command
    const startBrace = this.readBuffer.indexOf('{');

    if (startBrace === -1) {
      // No start brace yet - but capture any non-empty text as raw output
      if (this.readBuffer.length > 0) {
        if (ConnectionManager.messageCollector) {
          ConnectionManager.messageCollector.addMessage('received', this.readBuffer, 'ble');
        }
      }
      this.readBuffer = '';
      return;
    }

    // Capture any text BEFORE the start brace
    if (startBrace > 0) {
      const beforeText = this.readBuffer.substring(0, startBrace);
      if (ConnectionManager.messageCollector) {
        ConnectionManager.messageCollector.addMessage('received', beforeText, 'ble');
      }
      this.readBuffer = this.readBuffer.substring(startBrace);
    }

    // Now check if we have the closing brace
    const endBrace = this.readBuffer.indexOf('}');

    if (endBrace === -1) {
      // No end brace yet - keep buffering
      return;
    }

    // We have a complete pfod command from { to }
    const pfodString = this.readBuffer.substring(0, endBrace + 1);

    // Calculate time elapsed since send
    const receiveTime = Date.now();
    const elapsedMs = this.sendTime ? (receiveTime - this.sendTime) : 0;

    console.log(`[BLE_CONNECTION] Received complete pfod command after ${elapsedMs}ms:`, pfodString);

    // Convert pfod protocol to JSON format
    const jsonString = this.pfodToJson(pfodString);

    // Record the raw pfod message received
    if (ConnectionManager.messageCollector) {
      ConnectionManager.messageCollector.addMessage('received', pfodString, 'ble');
    }

    if (this.responseResolve) {
      // Cancel the timeout since we got a response
      if (this.timeoutId) {
        clearTimeout(this.timeoutId);
        this.timeoutId = null;
      }

      // Advance dedup character only on successful response (not on retry/timeout)
      advanceDedupChar();

      this.responseResolve(jsonString);
      this.responseResolve = null;
      this.responseReject = null;
    }

    // Clear the buffer up to and including the closing brace
    this.readBuffer = this.readBuffer.substring(endBrace + 1);
  }

  /**
   * Send a command via BLE and return the response
   * @param {string} cmd - The pfod command (e.g., "{.}" or "{dwgName}")
   * @returns {Promise<string>} - Response text (usually JSON)
   */
  async send(cmd) {
    // Auto-connect if not already connected
    if (!this.connected || !this.characteristicTX) {
      console.log('[BLE_CONNECTION] Not connected, connecting now...');
      await this.connect();
    }

    // Diagnostic logging: Check if a previous request is still pending
    if (this.responseResolve || this.responseReject) {
      console.warn(`[BLE_CONNECTION] WARNING: send() called while previous request still pending`);
      console.warn(`[BLE_CONNECTION] This should not happen - queue protection may not be working`);
    }

    // Cancel any previous timeout that might still be running
    if (this.timeoutId) {
      clearTimeout(this.timeoutId);
      console.log(`[BLE_CONNECTION] Cancelled previous timeout`);
      this.timeoutId = null;
    }

    // Clear previous response state
    this.readBuffer = '';

    // Record send time for performance measurement
    this.sendTime = Date.now();

    // Set up promise for response
    const responsePromise = new Promise((resolve, reject) => {
      this.responseResolve = resolve;
      this.responseReject = reject;

      // Get timeout from connection manager (default 10 seconds)
      const timeout = this.connectionManager.getResponseTimeout();
      console.log(`[BLE_CONNECTION] Setting response timeout to ${timeout}ms`);
      this.timeoutId = setTimeout(() => {
        if (this.responseReject) {
          this.responseReject(new Error('BLE response timeout - device may not be responding'));
          this.responseResolve = null;
          this.responseReject = null;
          this.timeoutId = null;
        }
      }, timeout);
    });

    // Prepend current dedup character to the command (don't advance yet - only advance on success)
    const cmdWithPrefix = getCurrentDedupChar() + cmd;

    // Send the command
    console.log(`[BLE_CONNECTION] Sending: ${cmdWithPrefix} at ${new Date(this.sendTime).toISOString()}`);

    // Record the command being sent (with the dedup prefix)
    if (ConnectionManager.messageCollector) {
      ConnectionManager.messageCollector.addMessage('sent', cmdWithPrefix, 'ble', cmd);
    }

    const encoder = new TextEncoder();
    const data = encoder.encode(cmdWithPrefix + '\n'); // Add newline for command termination
    await this.characteristicTX.writeValue(data);

    // Wait for response
    return responsePromise;
  }

  /**
   * Disconnect from the BLE device
   */
  async disconnect() {
    console.log('[BLE_CONNECTION] Disconnecting...');

    this.connected = false;

    try {
      // Stop notifications
      if (this.characteristicRX) {
        await this.characteristicRX.stopNotifications();
        console.log('[BLE_CONNECTION] Notifications stopped');
      }

      // Disconnect GATT server
      if (this.server && this.server.connected) {
        this.server.disconnect();
        console.log('[BLE_CONNECTION] GATT server disconnected');
      }

      // Clear references
      this.device = null;
      this.server = null;
      this.service = null;
      this.characteristicTX = null;
      this.characteristicRX = null;

      console.log('[BLE_CONNECTION] Disconnected successfully');
    } catch (error) {
      console.error('[BLE_CONNECTION] Error during disconnect:', error);
      throw error;
    }
  }

  /**
   * Check if BLE connection is active
   */
  isConnected() {
    return this.connected && this.server && this.server.connected;
  }
}

// Make classes available globally for browser use
window.ConnectionManager = ConnectionManager;
window.HTTPConnection = HTTPConnection;
window.SerialConnection = SerialConnection;
window.BLEConnection = BLEConnection;
