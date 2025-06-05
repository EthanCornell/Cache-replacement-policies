#!/bin/bash

# Large-Scale Cache Replacement Algorithm Testing Script
# Generates 20M+ page references and tests all algorithms

set -e  # Exit on any error

echo "=================================================="
echo "  Large-Scale Cache Replacement Algorithm Test"
echo "=================================================="
echo ""

# Configuration
DATA_DIR="large_test_data"
RESULTS_DIR="large_test_results"
CACHE_SIZES=(16 32 64 128 256)
WORKLOAD_SIZES=(20 50 100)  # MB

# Create directories
mkdir -p "$DATA_DIR"
mkdir -p "$RESULTS_DIR"

# Build the data generator
echo "Building data generator..."
g++ -std=c++20 -O3 -o data_generator large_data_generator.cpp
echo "✓ Data generator built"
echo ""

# Build the cache simulator
echo "Building cache simulator..."
make clean > /dev/null 2>&1
make > /dev/null 2>&1
echo "✓ Cache simulator built"
echo ""

# Function to generate test data
generate_workload() {
    local workload_type=$1
    local size_mb=$2
    local filename="$DATA_DIR/${workload_type}_${size_mb}mb.txt"
    
    if [ ! -f "$filename" ]; then
        echo "Generating $workload_type workload ($size_mb MB)..."
        ./data_generator "$workload_type" "$filename" "$size_mb"
    else
        echo "✓ $workload_type workload ($size_mb MB) already exists"
    fi
}

# Function to test cache performance
test_cache_performance() {
    local workload_file=$1
    local cache_size=$2
    local workload_name=$3
    local size_mb=$4
    
    echo "Testing $workload_name with $cache_size frames..."
    
    local result_file="$RESULTS_DIR/${workload_name}_${size_mb}mb_${cache_size}frames.txt"
    
    # Run with timeout to prevent infinite loops
    timeout 1800 ./cache_simulator "$workload_file" a "$cache_size" 0 0 > "$result_file" 2>&1 || {
        echo "⚠️  Test timed out or failed for $workload_name (${cache_size} frames)"
        echo "Timeout or error occurred" > "$result_file"
        return 1
    }
    
    echo "✓ Completed $workload_name with $cache_size frames"
}

# Function to extract performance metrics
extract_metrics() {
    local result_file=$1
    local algorithm=$2
    
    # Extract hit ratio and execution time for specific algorithm
    grep -A 1 "^$algorithm Algorithm" "$result_file" | tail -1 | \
    awk '{print $6 "," $10}' | sed 's/,$//'
}

# Function to generate performance summary
generate_summary() {
    local workload_name=$1
    local size_mb=$2
    
    echo ""
    echo "Performance Summary: $workload_name ($size_mb MB)"
    echo "=============================================="
    printf "%-12s %-8s %-10s %-12s %-12s %-12s %-12s\n" \
           "Algorithm" "Frames" "Hit Ratio" "Time (s)" "Misses" "Throughput" "Efficiency"
    echo "--------------------------------------------------------------------------------"
    
    for cache_size in "${CACHE_SIZES[@]}"; do
        local result_file="$RESULTS_DIR/${workload_name}_${size_mb}mb_${cache_size}frames.txt"
        
        if [ -f "$result_file" ] && [ -s "$result_file" ]; then
            # Extract metrics for each algorithm
            for algorithm in "OPTIMAL" "LRU" "FIFO" "CLOCK" "RANDOM" "LFU" "LFRU"; do
                local line=$(grep -A 1 "^$algorithm Algorithm" "$result_file" 2>/dev/null | tail -1)
                if [ -n "$line" ]; then
                    local hits=$(echo "$line" | awk '{print $4}' | sed 's/,$//')
                    local misses=$(echo "$line" | awk '{print $6}' | sed 's/,$//')
                    local hit_ratio=$(echo "$line" | awk '{print $8}' | sed 's/,$//')
                    local exec_time=$(echo "$line" | awk '{print $12}')
                    
                    # Calculate throughput (references per second)
                    local total_refs=$((hits + misses))
                    local throughput=$(echo "scale=0; $total_refs / $exec_time" | bc -l 2>/dev/null || echo "N/A")
                    
                    # Calculate efficiency (hit ratio / time)
                    local efficiency=$(echo "scale=4; $hit_ratio / $exec_time" | bc -l 2>/dev/null || echo "N/A")
                    
                    printf "%-12s %-8s %-10s %-12s %-12s %-12s %-12s\n" \
                           "$algorithm" "$cache_size" "$hit_ratio" "$exec_time" "$misses" "$throughput" "$efficiency"
                fi
            done
            echo ""
        fi
    done
}

# Main testing workflow
main() {
    echo "Step 1: Generating test workloads..."
    echo "===================================="
    
    # Generate different workload types
    for size in "${WORKLOAD_SIZES[@]}"; do
        generate_workload "locality" "$size"
        generate_workload "random" "$size"
        generate_workload "sequential" "$size"
        generate_workload "adversarial" "$size"
        generate_workload "realistic" "$size"
    done
    
    echo ""
    echo "Step 2: Running performance tests..."
    echo "===================================="
    
    # Test each workload with different cache sizes
    for size in "${WORKLOAD_SIZES[@]}"; do
        for workload in "locality" "random" "sequential" "adversarial" "realistic"; do
            echo ""
            echo "Testing $workload workload ($size MB)..."
            
            for cache_size in "${CACHE_SIZES[@]}"; do
                local workload_file="$DATA_DIR/${workload}_${size}mb.txt"
                test_cache_performance "$workload_file" "$cache_size" "$workload" "$size"
            done
        done
    done
    
    echo ""
    echo "Step 3: Generating performance summaries..."
    echo "==========================================="
    
    # Generate summaries for each workload
    for size in "${WORKLOAD_SIZES[@]}"; do
        for workload in "locality" "random" "sequential" "adversarial" "realistic"; do
            generate_summary "$workload" "$size" | tee "$RESULTS_DIR/summary_${workload}_${size}mb.txt"
        done
    done
    
    echo ""
    echo "Step 4: Overall analysis..."
    echo "==========================="
    
    # Find best performing algorithms for each workload type
    echo "Best Performing Algorithms by Workload:"
    echo "========================================"
    
    for workload in "locality" "random" "sequential" "adversarial" "realistic"; do
        echo ""
        echo "$workload workload:"
        
        # Find algorithm with highest average hit ratio across all sizes and cache sizes
        for algorithm in "OPTIMAL" "LRU" "FIFO" "CLOCK" "RANDOM" "LFU" "LFRU"; do
            local total_hit_ratio=0
            local count=0
            
            for size in "${WORKLOAD_SIZES[@]}"; do
                for cache_size in "${CACHE_SIZES[@]}"; do
                    local result_file="$RESULTS_DIR/${workload}_${size}mb_${cache_size}frames.txt"
                    if [ -f "$result_file" ]; then
                        local hit_ratio=$(grep -A 1 "^$algorithm Algorithm" "$result_file" 2>/dev/null | tail -1 | awk '{print $8}' | sed 's/,$//')
                        if [ -n "$hit_ratio" ]; then
                            total_hit_ratio=$(echo "$total_hit_ratio + $hit_ratio" | bc -l)
                            count=$((count + 1))
                        fi
                    fi
                done
            done
            
            if [ $count -gt 0 ]; then
                local avg_hit_ratio=$(echo "scale=6; $total_hit_ratio / $count" | bc -l)
                echo "  $algorithm: $avg_hit_ratio (avg hit ratio)"
            fi
        done
    done
    
    echo ""
    echo "Testing completed!"
    echo "=================="
    echo "Results saved in: $RESULTS_DIR/"
    echo "Data files saved in: $DATA_DIR/"
    echo ""
    echo "Key findings:"
    echo "- Check summary files for detailed performance metrics"
    echo "- OPTIMAL should always perform best (theoretical limit)"
    echo "- LRU typically performs well for locality workloads"
    echo "- FIFO may struggle with adversarial patterns"
    echo "- RANDOM provides baseline performance"
    echo ""
    echo "Disk usage:"
    du -sh "$DATA_DIR" "$RESULTS_DIR"
}

# Check dependencies
check_dependencies() {
    local missing_deps=()
    
    command -v g++ >/dev/null 2>&1 || missing_deps+=("g++")
    command -v make >/dev/null 2>&1 || missing_deps+=("make")
    command -v bc >/dev/null 2>&1 || missing_deps+=("bc")
    command -v timeout >/dev/null 2>&1 || missing_deps+=("timeout")
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        echo "Error: Missing dependencies: ${missing_deps[*]}"
        echo "Please install the missing tools and try again."
        exit 1
    fi
}

# Cleanup function
cleanup() {
    echo ""
    echo "Cleaning up temporary files..."
    # Keep data and results but clean build artifacts
    rm -f data_generator
}

# Set up signal handlers
trap cleanup EXIT

# Run the main test suite
check_dependencies
main

echo "Large-scale testing completed successfully!"