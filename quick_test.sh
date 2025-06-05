#!/bin/bash

# Quick test script to verify the cache simulator works
echo "=== Quick Cache Simulator Test ==="

# Create test input
echo "Creating test input file..."
cat > quick_test_input.txt << EOF
1 0
1 1
1 2
1 0
1 3
1 0
1 4
1 2
1 3
1 0
EOF

echo "Built files:"
ls -la cache_simulator test_cache 2>/dev/null || echo "Build files first with 'make'"

echo ""
echo "Testing LRU algorithm with 3 frames:"
./cache_simulator quick_test_input.txt L 3 0 0

echo ""
echo "Testing all algorithms with 3 frames:"
./cache_simulator quick_test_input.txt a 3 0 0

# Cleanup
rm -f quick_test_input.txt

echo ""
echo "=== Test Complete ==="