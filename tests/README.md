# Menu System Test Suite

This directory contains the comprehensive test suite for the menu system. It includes unit tests, integration tests, performance benchmarks, and memory profiling.

## Test Organization

### Unit Tests
- `test_menu.c`: Tests core menu functionality
- `test_menu_manager.c`: Tests menu management and lifecycle

### Integration Tests
- `test_integration.c`: Tests component interactions and workflows

### Performance Tests
- `test_performance.c`: Benchmarks critical operations
- Measures response times and throughput

### Memory Tests
- `test_memory.c`: Tracks memory usage and detects leaks
- Verifies memory stability during operations

## Running Tests

### Basic Test Execution
```bash
# Run all tests
./tests/run_tests.sh

# Run specific test
make test-menu
make test-menu_manager
make test-integration
make test-performance
make test-memory
```

### Test Configurations

#### Normal Mode
```bash
make tests
```
Runs tests with standard configuration.

#### Sanitizer Mode
```bash
make test-sanitize
```
Runs tests with Address and UB sanitizers enabled.

#### Coverage Mode
```bash
make test-coverage
```
Generates coverage reports in HTML format.

## Test Reports

### Generating Reports
```bash
# Generate comprehensive test report
python3 tests/generate_report.py \
    unit.log \
    integration.log \
    performance.log \
    coverage.log
```

### Report Components
- Test execution results
- Performance metrics
- Code coverage data
- Memory analysis
- Historical trends

## CI/CD Integration

The test suite is integrated with GitHub Actions:
- Runs on every push and pull request
- Executes all test configurations
- Generates and uploads reports
- Tracks performance metrics

## Memory Testing

### Running Memory Tests
```bash
# Run with Valgrind
make test-memory VALGRIND=1

# Run with Address Sanitizer
make test-memory SANITIZE=address

# Generate detailed memory profile
make test-memory PROFILE=1
```

### Memory Test Components
1. Allocation tracking
2. Peak usage monitoring
3. Leak detection
4. Memory pattern analysis

## Performance Testing

### Running Benchmarks
```bash
# Run all benchmarks
make test-performance

# Run specific benchmark
make bench-menu
make bench-navigation
make bench-input
```

### Benchmark Metrics
- Menu creation/destruction time
- Navigation response time
- Input handling latency
- Memory allocation patterns

## Test Development

### Adding New Tests

1. Create test file in appropriate category:
```c
// tests/test_new_feature.c
#include "../src/new_feature.h"
#include <assert.h>

static void test_feature() {
    // Test implementation
}

int main(void) {
    test_feature();
    return 0;
}
```

2. Add to Makefile:
```makefile
test-new_feature: $(TEST_BUILD_DIR)/test_new_feature
    ./$(TEST_BUILD_DIR)/test_new_feature
```

3. Update run_tests.sh to include new test

### Test Guidelines

1. **Organization**
   - Group related tests
   - Clear test names
   - Descriptive failure messages

2. **Setup/Teardown**
   - Clean resource management
   - Isolated test environment
   - Proper cleanup

3. **Assertions**
   - Specific failure messages
   - Complete state verification
   - Edge case coverage

4. **Documentation**
   - Purpose description
   - Requirements tested
   - Expected outcomes

## Debugging Tests

### Tools and Techniques

1. **GDB Integration**
```bash
# Run test under GDB
make test-debug TEST=menu
```

2. **Logging**
```bash
# Enable verbose output
make tests VERBOSE=1
```

3. **Sanitizers**
```bash
# Run with specific sanitizer
make tests SANITIZER=address
make tests SANITIZER=undefined
```

## Common Issues

### Memory Leaks
- Use Valgrind/ASan for detection
- Check cleanup in teardown
- Verify resource release

### Test Failures
1. Check test environment
2. Verify X11 connection
3. Examine test logs
4. Run with debug output

### Performance Issues
1. Use perf for profiling
2. Check system resources
3. Isolate slow operations

## Contributing

1. Follow test naming conventions
2. Include documentation
3. Add to CI configuration
4. Update test reports
5. Verify all tests pass

## Support

For issues or questions:
1. Check existing issues
2. Run tests with debug output
3. Include test logs
4. Provide system details