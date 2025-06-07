# Integration Tests

This directory contains integration tests that interact with external systems and files on the filesystem.

## OTEL Filesystem Tests (`otel_filesystem_test.cpp`)

These tests validate the OpenTelemetry integration by reading configuration files from the local OTEL setup at `$HOME/git/local-otel`.

### What These Tests Do

1. **Directory Structure Validation**
   - Verifies that the `$HOME/git/local-otel` directory exists
   - Checks for expected subdirectories (`configs`, `data`, etc.)
   - Logs directory contents for debugging

2. **Configuration File Access**
   - Tests reading the OTEL collector configuration file
   - Validates file permissions and accessibility
   - Checks file size and basic structure

3. **YAML Configuration Parsing**
   - Reads the `wasm-collector.yaml` configuration
   - Validates OTEL collector structure (receivers, processors, exporters, service)
   - Checks for required OTLP endpoints and ports (4317, 4318)

4. **File Content Validation**
   - Reads configuration files line by line
   - Validates YAML keywords and structure
   - Ensures configuration follows OTEL collector format

5. **Data Directory Exploration**
   - Checks for optional data output directories
   - Counts files in data directories
   - Logs directory structure for debugging

### Test Behavior

- **Graceful Skipping**: Tests automatically skip if the `$HOME/git/local-otel` directory doesn't exist
- **Environment-Aware**: Uses the `HOME` environment variable to locate the OTEL directory
- **Non-Destructive**: Only reads files, never modifies or deletes anything
- **Comprehensive Logging**: Provides detailed logging for debugging test issues

### Prerequisites

For these tests to run successfully, you need:

1. The `$HOME/git/local-otel` directory to exist
2. A valid OTEL collector configuration at `$HOME/git/local-otel/configs/wasm-collector.yaml`
3. Read permissions on the OTEL directory and files

### Running the Tests

```bash
# Build and run all tests
cd build
cmake --build . --target finch-tests
./test/bin/finch-tests --gtest_filter="OtelFilesystemTest.*"

# Run only integration tests
./test/bin/finch-tests --gtest_filter="*Integration*"

# Run with verbose output
./test/bin/finch-tests --gtest_filter="OtelFilesystemTest.*" --gtest_also_run_disabled_tests
```

### Test Output

The tests will provide detailed logging output showing:

- Directory structure found
- Files discovered
- Configuration validation results
- Any issues encountered

Example output:

```
[INFO] Found OTEL directory at: /Users/username/git/local-otel
[INFO]   Directory: configs
[INFO]   File: README.md
[INFO]   File: docker-compose.yml
[INFO] Config file size: 1234 bytes
[INFO] Successfully read 45 lines from config file
[INFO] Found YAML config: wasm-collector.yaml
[INFO] OTEL configuration validation passed
```

### Troubleshooting

If tests are skipping:

1. Verify `$HOME/git/local-otel` exists
2. Check that the `configs` subdirectory exists
3. Ensure `wasm-collector.yaml` is present
4. Verify read permissions on files

If tests are failing:

1. Check the YAML file format and syntax
2. Verify OTEL collector configuration structure
3. Ensure required sections (receivers, processors, exporters, service) exist
4. Check that OTLP endpoints are configured correctly
