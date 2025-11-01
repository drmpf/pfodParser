/*
   caching.js
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */

/**
 * Response Caching Module
 * Handles caching of pfod responses with version tracking
 */

// Get unique identifier for current connection
// Returns IP address for HTTP, device name for BLE, or 'Serial' for serial connections
function getConnectionIdentifier(connectionManager) {
  if (!connectionManager) {
    return 'unknown_connection';
  }

  switch(connectionManager.protocol) {
    case 'http':
      return connectionManager.config.targetIP || 'unknown_ip';
    case 'ble':
      // BLE device name from config if available
      return connectionManager.config.deviceName || connectionManager.config.deviceId || 'unknown_ble';
    case 'serial':
      return 'Serial';
    default:
      return 'unknown_connection';
  }
}

// Determine response type from msgType (cmd[0])
// Returns: 'menuStart', 'menuUpdate', 'dwgStart', 'dwgUpdate', or null
function getResponseType(msgType) {
  if (!msgType || typeof msgType !== 'string') {
    return null;
  }

  if (msgType.startsWith('{,')) {
    return 'menuStart';
  }
  if (msgType.startsWith('{;')) {
    return 'menuUpdate';
  }
  if (msgType === '{+') {
    return 'dwgUpdate';
  }
  if (msgType.startsWith('{+') && msgType.length > 2) {
    return 'dwgStart';
  }

  return null;
}

// Extract command key from request
// Format: {[version:]commandKey[separator...]}
// Separators: backtick, tilde, space, closing brace
// Examples:
//   {v1:.} → "."
//   {v1:c1`data} → "c1"
//   {v1:c1~data} → "c1"
//   {c1`data} → "c1"
//   {.} → "."
function getCommandKeyFromRequest(request) {
  if (!request) {
    return null;
  }

  const cmd = request.cmd;
  if (!cmd || typeof cmd !== 'string') {
    return null;
  }

  // Skip the opening {
  let content = cmd.substring(1);

  // Check if there's a version prefix (indicated by :)
  const colonIndex = content.indexOf(':');
  if (colonIndex !== -1) {
    // Skip the version part
    content = content.substring(colonIndex + 1);
  }

  // Extract the command key up to the next separator or closing brace
  // Separators are: backtick, tilde, space, closing brace
  const match = content.match(/^([^`~\s}]+)/);
  if (match && match[1]) {
    return match[1];
  }

  console.log('[CACHE] Could not extract command key from request:', cmd);
  return null;
}

// Extract version from msgType string (cmd[0])
// Version is the last string after the second ~ if present
// Returns null if no version found or fewer than 2 tildes
function extractVersionFromResponse(msgType) {
  if (!msgType || typeof msgType !== 'string') {
    return null;
  }

  // Count tildes to find version (after second ~)
  const tildeIndices = [];
  for (let i = 0; i < msgType.length; i++) {
    if (msgType[i] === '~') {
      tildeIndices.push(i);
    }
  }

  // Need at least 2 tildes for a version
  if (tildeIndices.length < 2) {
    console.log('[CACHE] No version found in msgType (fewer than 2 tildes)');
    return null;
  }

  // Get everything after the second ~
  const afterSecondTilde = msgType.substring(tildeIndices[1] + 1).trim();

  if (afterSecondTilde.length === 0) {
    console.log('[CACHE] No version found after second tilde');
    return null;
  }

  // Extract the version string (typically a single token like V1, V2, etc.)
  const versionMatch = afterSecondTilde.match(/^\S+/);
  if (versionMatch) {
    const version = versionMatch[0];
    console.log(`[CACHE] Extracted version: ${version}`);
    return version;
  }

  console.log('[CACHE] Could not parse version from:', afterSecondTilde);
  return null;
}

// Generate cache key for storing response
// Format: pfodWeb_cache_{connection_identifier}_{commandKey}
function getCacheKey(connectionId, commandKey) {
  return `pfodWeb_cache_${connectionId}_${commandKey}`;
}

// Cache response to localStorage
// Only caches if version is present and response is a cacheable type
// Uses the command key from the request (what was sent)
// Extracts msgType from data.cmd[0]
function cacheResponse(data, request, connectionManager) {
  try {
    // Extract msgType from response data
    if (!data || !data.cmd || !Array.isArray(data.cmd) || data.cmd.length === 0) {
      console.log('[CACHE] Not caching - no cmd data in response');
      return;
    }

    const msgType = data.cmd[0];

    // Determine response type (to validate it's cacheable)
    const responseType = getResponseType(msgType);
    if (!responseType) {
      console.log('[CACHE] Not caching - response type not recognized');
      return;
    }

    // Extract version
    const version = extractVersionFromResponse(msgType);
    if (!version) {
      console.log(`[CACHE] Not caching ${responseType} - no version found in response`);
      return;
    }

    // Extract command key from request
    const commandKey = getCommandKeyFromRequest(request);
    if (!commandKey) {
      console.log('[CACHE] Not caching - could not extract command key from request');
      return;
    }

    // Add version to data object
    data.version = version;

    // Get connection identifier
    const connectionId = getConnectionIdentifier(connectionManager);

    // Generate cache key
    const cacheKey = getCacheKey(connectionId, commandKey);

    // Store to localStorage
    localStorage.setItem(cacheKey, JSON.stringify(data));
    console.log(`[CACHE] Cached ${responseType} (cmd key: ${commandKey}) for connection "${connectionId}" with version "${version}". Key: ${cacheKey}`);
  } catch (e) {
    console.error('[CACHE] Error caching response:', e);
  }
}
