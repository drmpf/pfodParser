#!/usr/bin/env node
/**
 * build-bundle.js
 * Combines all pfodWeb files into single standalone HTML files
 *
 * Usage: node build-bundle.js
 *
 * Creates in parent directory:
 *   - index.html (combined with inlined JS)
 *   - pfodWeb.html (combined with inlined JS)
 *   - pfodWebDebug.html (combined with inlined JS)
 *
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 */

const fs = require('fs');
const path = require('path');

// Configuration
const config = {
  sourceDir: __dirname,
  outputDir: path.join(__dirname, '..'),

  // Files to bundle
  bundles: [
    {
      name: 'pfodWeb.html',
      template: 'pfodWeb.html',
      scripts: [
        'version.js',
        'connectionManager.js',
        'caching.js',
        'messageViewer.js',
        'pfodWebDebug.js',
        'DrawingManager.js',
        'displayTextUtils.js',
        'redraw.js',
        'drawingMerger.js',
        'webTranslator.js',
        'drawingDataProcessor.js',
        'pfodWebMouse.js',
        'pfodWeb.js'
      ]
    },
    {
      name: 'pfodWebDebug.html',
      template: 'pfodWebDebug.html',
      scripts: [
        'version.js',
        'connectionManager.js',
        'caching.js',
        'messageViewer.js',
        'pfodWebDebug.js',
        'DrawingManager.js',
        'displayTextUtils.js',
        'redraw.js',
        'drawingMerger.js',
        'webTranslator.js',
        'drawingDataProcessor.js',
        'pfodWebMouse.js'
      ]
    },
    {
      name: 'index.html',
      template: 'index.html',
      scripts: [
        'version.js'
      ]
    }
  ]
};

/**
 * Read file with error handling
 */
function readFile(filePath) {
  try {
    const fullPath = path.join(config.sourceDir, filePath);
    const content = fs.readFileSync(fullPath, 'utf8');
    console.log(`  ✓ Read ${filePath} (${content.length} bytes)`);
    return content;
  } catch (error) {
    console.error(`  ✗ Error reading ${filePath}:`, error.message);
    throw error;
  }
}

/**
 * Read binary file and convert to base64
 */
function readBinaryFileAsBase64(filePath) {
  try {
    const fullPath = path.join(config.sourceDir, filePath);
    const buffer = fs.readFileSync(fullPath);
    const base64 = buffer.toString('base64');
    console.log(`  ✓ Read ${filePath} (${buffer.length} bytes, converted to base64)`);
    return base64;
  } catch (error) {
    console.error(`  ✗ Error reading ${filePath}:`, error.message);
    throw error;
  }
}

/**
 * Embed favicon into HTML head
 */
function embedFavicon(htmlContent, faviconBase64) {
  // Create favicon link tag with data URI
  const faviconLink = `<link rel="icon" type="image/x-icon" href="data:image/x-icon;base64,${faviconBase64}">`;

  // Find the </head> tag and insert favicon before it
  const headCloseIndex = htmlContent.indexOf('</head>');

  if (headCloseIndex !== -1) {
    // Insert before </head>
    return htmlContent.substring(0, headCloseIndex) + faviconLink + '\n' + htmlContent.substring(headCloseIndex);
  } else {
    // If no </head>, find <body> and insert before it
    const bodyIndex = htmlContent.indexOf('<body');
    if (bodyIndex !== -1) {
      return htmlContent.substring(0, bodyIndex) + faviconLink + '\n' + htmlContent.substring(bodyIndex);
    }
  }

  // Fallback: insert at beginning after <!DOCTYPE or <html
  const htmlTagIndex = htmlContent.indexOf('<html');
  if (htmlTagIndex !== -1) {
    const nextTagIndex = htmlContent.indexOf('>', htmlTagIndex);
    return htmlContent.substring(0, nextTagIndex + 1) + '\n' + faviconLink + '\n' + htmlContent.substring(nextTagIndex + 1);
  }

  // Last resort: prepend to content
  return faviconLink + '\n' + htmlContent;
}

/**
 * Inline JavaScript files into HTML
 */
function inlineScripts(htmlContent, scriptFiles) {
  let result = htmlContent;

  // First, collect all JavaScript content
  let scriptsContent = scriptFiles.map(scriptFile => {
    const jsContent = readFile(scriptFile);
    return `\n/* ========================================\n * Inlined from: ${scriptFile}\n * ======================================== */\n${jsContent}`;
  }).join('\n\n');

  // Comment out the loadDependencies() and loadDependencies_noDebug() calls in bundled version
  // This prevents dynamic script loading since all scripts are already inlined
  scriptsContent = scriptsContent.replace(
    /(\s+)(await loadDependencies(?:_noDebug)?\(\);)/g,
    '$1// $2 // Commented out in bundled version - all scripts already inlined'
  );

  // Remove ALL external script tags with src attribute
  // Pattern matches: <script...src="..."...></script> with any whitespace
  result = result.replace(/<script[^>]*src\s*=\s*["'][^"']*["'][^>]*>\s*<\/script>/gi, '');

  // Find the first inline <script> tag (if any) or fall back to </body>
  // This ensures inlined scripts are loaded BEFORE any existing inline scripts
  const firstScriptIndex = result.indexOf('<script');
  const bodyCloseIndex = result.lastIndexOf('</body>');

  let insertIndex;
  if (firstScriptIndex !== -1 && firstScriptIndex < bodyCloseIndex) {
    // Insert before first inline script tag
    insertIndex = firstScriptIndex;
  } else if (bodyCloseIndex !== -1) {
    // No inline scripts, insert before </body>
    insertIndex = bodyCloseIndex;
  } else {
    // No </body> tag, append at end
    insertIndex = -1;
  }

  const inlinedScript = `\n<!-- All JavaScript files combined inline -->\n<script>\n// Set flag to indicate this is a bundled version (prevents dynamic script loading)\nwindow.PFODWEB_BUNDLED = true;\n\n${scriptsContent}\n</script>\n`;

  if (insertIndex !== -1) {
    result = result.substring(0, insertIndex) + inlinedScript + result.substring(insertIndex);
  } else {
    result += inlinedScript;
  }

  return result;
}

/**
 * Add banner comment to output file
 */
function addBanner(content, bundleName) {
  const banner = `<!--
================================================================================
  STANDALONE BUNDLE: ${bundleName}
  Generated: ${new Date().toISOString()}

  This file contains all JavaScript inlined for standalone deployment.
  No external files or webserver required - just open in browser!

  For development, edit the separate source files and rebuild.

  Build command: node build-bundle.js

  (c)2025 Forward Computing and Control Pty. Ltd.
  NSW Australia, www.forward.com.au
================================================================================
-->
`;
  return banner + content;
}

/**
 * Create a single bundle
 */
function createBundle(bundleConfig, faviconBase64) {
  console.log(`\nCreating bundle: ${bundleConfig.name}`);
  console.log(`  Template: ${bundleConfig.template}`);
  console.log(`  Scripts: ${bundleConfig.scripts.length} files`);

  // Read template HTML
  const templateContent = readFile(bundleConfig.template);

  // Embed favicon
  let bundledContent = embedFavicon(templateContent, faviconBase64);

  // Inline all scripts
  bundledContent = inlineScripts(bundledContent, bundleConfig.scripts);

  // Add banner
  bundledContent = addBanner(bundledContent, bundleConfig.name);

  // Write output file
  const outputPath = path.join(config.outputDir, bundleConfig.name);
  fs.writeFileSync(outputPath, bundledContent, 'utf8');

  const size = (bundledContent.length / 1024).toFixed(2);
  console.log(`  ✓ Created ${bundleConfig.name} (${size} KB)`);

  return outputPath;
}

/**
 * Main build process
 */
function build() {
  console.log('========================================');
  console.log('  pfodWeb Bundle Builder');
  console.log('========================================');

  // Read favicon once at the start
  console.log('\nPreparing resources:');
  let faviconBase64;
  try {
    faviconBase64 = readBinaryFileAsBase64('favicon.ico');
  } catch (error) {
    console.error('\n✗ Failed to read favicon.ico:');
    console.error(error.message);
    process.exit(1);
  }

  // Create each bundle
  const outputs = [];
  for (const bundleConfig of config.bundles) {
    try {
      const outputPath = createBundle(bundleConfig, faviconBase64);
      outputs.push(outputPath);
    } catch (error) {
      console.error(`\n✗ Failed to create ${bundleConfig.name}:`, error.message);
      process.exit(1);
    }
  }

  // Summary
  console.log('\n========================================');
  console.log('  Build Complete!');
  console.log('========================================');
  console.log('\nGenerated files:');
  outputs.forEach(file => {
    const stats = fs.statSync(file);
    const size = (stats.size / 1024).toFixed(2);
    console.log(`  • ${path.basename(file)} (${size} KB)`);
  });

  console.log('\nDeployment:');
  console.log('  1. Copy HTML files to deployment location');
  console.log('  2. Double-click index.html to launch');
  console.log('  3. No webserver needed - runs from file://');
  console.log('\nNote: Device must have CORS headers enabled for HTTP connections');
  console.log('');
}

// Run build
if (require.main === module) {
  build();
}

module.exports = { build };
