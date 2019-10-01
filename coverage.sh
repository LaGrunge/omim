#!/bin/sh
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info # filter system-files
lcov --list coverage.info # debug info
# Uploading report to CodeCov
curl -s https://codecov.io/bash | bash -s -- -f coverage.info || echo "Codecov did not collect coverage reports"
