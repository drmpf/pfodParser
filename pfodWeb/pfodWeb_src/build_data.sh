#!/bin/bash
# Unix shell script to gzip JavaScript and HTML files individually
# Creates separate .gz files for each source file in ../data directory

echo "Compressing JavaScript and HTML files with gzip..."
echo ""

# Create ../data directory if it doesn't exist
if [ ! -d "../data" ]; then
    echo "Creating ../data directory..."
    mkdir -p "../data"
    echo ""
fi

# Check if gzip is available
if ! command -v gzip &> /dev/null; then
    echo "Error: gzip command not found!"
    echo "Please install gzip package"
    exit 1
fi

# Function to compress file and show results
compress_file() {
    local file="$1"

    if [ -f "$file" ]; then
        echo "  Compressing $file..."

        # Use gzip -c to compress to stdout, redirect to .gz file in ../data directory
        if gzip -c "$file" > "../data/$file.gz"; then
            # Get file sizes
            original_size=$(stat -c%s "$file" 2>/dev/null || stat -f%z "$file" 2>/dev/null || echo "0")
            compressed_size=$(stat -c%s "../data/$file.gz" 2>/dev/null || stat -f%z "../data/$file.gz" 2>/dev/null || echo "0")

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

# Compress all eligible files (.js, .html, .ico)
echo "Compressing files..."
count=0
for file in *.js *.html *.ico 2>/dev/null; do
    # Skip if file doesn't exist (in case no matches)
    [ -f "$file" ] || continue

    # Skip script files and markdown files
    case "$file" in
        *.sh|*.bat|*.md)
            continue
            ;;
    esac

    if compress_file "$file"; then
        ((count++))
    fi
done

echo ""
echo "Gzip compression completed!"
echo "Total files processed: $count"
echo ""

# Show all created .gz files
if [ -f "../data"/*.js.gz ] || [ -f "../data"/*.html.gz ]; then
    echo "Created .gz files in ../data:"
    ls -la ../data/*.js.gz ../data/*.html.gz 2>/dev/null | awk '{printf "  %s (%s bytes)\n", $9, $5}'

    echo ""
    echo "Total .gz files created: $(ls -1 ../data/*.js.gz ../data/*.html.gz 2>/dev/null | wc -l)"
else
    echo "No .gz files were created"
    exit 1
fi

echo ""
echo "Performing post-compression cleanup..."
echo ""

# Delete build-bundle.js.gz if it exists
if [ -f "../data/build-bundle.js.gz" ]; then
    echo "Deleting build-bundle.js.gz..."
    rm "../data/build-bundle.js.gz"
fi

# Delete index.html.gz if it exists
if [ -f "../data/index.html.gz" ]; then
    echo "Deleting index.html.gz..."
    rm "../data/index.html.gz"
fi

# Rename data_index.html.gz to index.html.gz if it exists
if [ -f "../data/data_index.html.gz" ]; then
    echo "Renaming data_index.html.gz to index.html.gz..."
    mv "../data/data_index.html.gz" "../data/index.html.gz"
fi

echo "Cleanup complete!"
echo ""
echo "Files are ready for web server deployment with Content-Encoding: gzip header"