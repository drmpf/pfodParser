#!/bin/bash
# Unix shell script to gzip JavaScript and HTML files individually
# Creates separate .gz files for each source file

echo "Compressing JavaScript and HTML files with gzip..."
echo ""

# Check if gzip is available
if ! command -v gzip &> /dev/null; then
    echo "Error: gzip command not found!"
    echo "Please install gzip package"
    exit 1
fi

# JavaScript files array
js_files=(
    "pfodWebMouse.js"
    "webTranslator.js"
    "DrawingManager.js"
    "drawingDataProcessor.js"
    "redraw.js"
    "pfodWebDebug.js"
    "mergeAndRedraw.js"
    "pfodWeb.js"
    "displayTextUtils.js"
)

# HTML files array
html_files=(
    "index.html"
    "localIndex.html"
    "pfodWebDebug.html"
    "pfodWeb.html"
)

# Function to compress file and show results
compress_file() {
    local file="$1"
    local type="$2"

    if [ -f "$file" ]; then
        echo "  Compressing $file..."

        # Use gzip -c to compress to stdout, redirect to .gz file
        if gzip -c "$file" > "$file.gz"; then
            # Get file sizes
            original_size=$(stat -c%s "$file" 2>/dev/null || echo "0")
            compressed_size=$(stat -c%s "$file.gz" 2>/dev/null || echo "0")

            if [ "$original_size" -gt 0 ] && [ "$compressed_size" -gt 0 ]; then
                # Calculate compression ratio
                ratio=$(echo "scale=1; (1 - $compressed_size / $original_size) * 100" | bc 2>/dev/null || echo "0")
                echo "    ✓ Created $file.gz ($original_size → $compressed_size bytes, ${ratio}% compression)"
            else
                echo "    ✓ Created $file.gz"
            fi
        else
            echo "    ✗ Failed to create $file.gz"
            return 1
        fi
    else
        echo "    ⚠ Warning: $file not found"
        return 1
    fi
}

# Compress JavaScript files
echo "Compressing JavaScript files:"
js_success=0
js_total=0
for file in "${js_files[@]}"; do
    ((js_total++))
    if compress_file "$file" "JavaScript"; then
        ((js_success++))
    fi
done

echo ""

# Compress HTML files
echo "Compressing HTML files:"
html_success=0
html_total=0
for file in "${html_files[@]}"; do
    ((html_total++))
    if compress_file "$file" "HTML"; then
        ((html_success++))
    fi
done

echo ""
echo "Gzip compression completed!"
echo "JavaScript files: $js_success/$js_total successful"
echo "HTML files: $html_success/$html_total successful"
echo ""

# Show all created .gz files
gz_files=(*.js.gz *.html.gz)
if [ -f "${gz_files[0]}" ]; then
    echo "Created .gz files:"
    ls -la *.js.gz *.html.gz 2>/dev/null | awk '{printf "  %s (%s bytes)\n", $9, $5}'

    echo ""
    echo "Total .gz files created: $(ls -1 *.js.gz *.html.gz 2>/dev/null | wc -l)"
else
    echo "No .gz files were created"
    exit 1
fi

echo ""
echo "Files are ready for web server deployment with Content-Encoding: gzip header"