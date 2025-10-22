// Shared constants to avoid circular dependencies
  var JS_VERSION = "V2.0.0 -- 13th October 2025";
// V2.0.0 removed nodejs server, bundled all files in single htmls
// V1.1.5 added init() of drawings
// V1.1.4 added pfodMainDrawing.h generated file
// V1.1.3 dwg updates as response received

// Make available globally for browser use
if (typeof window !== 'undefined') {
    window.JS_VERSION = JS_VERSION;
}

// Export for Node.js use
if (typeof module !== 'undefined' && module.exports) {
    module.exports = { JS_VERSION };
}