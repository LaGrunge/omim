#!/bin/bash
# Please run this script to configure the repository after cloning it.

# When configuring with private repository, the following override hierarchy is used:
# - commandline parameters - most specific, always wins.
# - stdin parameters.
# - saved repository - least specific, if present.
# - fallback to opensource mode.

# Stop on the first error.
set -e -u

BASE_PATH=$(cd "$(dirname "$0")"; pwd)

PRIVATE_HEADER="$BASE_PATH/private.h"
DEFAULT_PRIVATE_HEADER="$BASE_PATH/private_default.h"
PRIVATE_CAR_MODEL_COEFS="$BASE_PATH/routing_common/car_model_coefs.hpp"
DEFAULT_PRIVATE_CAR_MODEL_COEFS="$BASE_PATH/routing_common/car_model_coefs_default.hpp"

usage() {
  echo "This tool configures omim with private repository or as an opensource build"
  echo "Usage:"
  echo "  $0 private_repo_url [private_repo_branch]  - to configure with private repository"
  echo "  echo '[private_repo_url] [private_repo_branch]' | $0  - alternate invocation for private repository configuration"
  echo "  $0  - to use with saved repository url and branch or to set up an open source build if nothing is saved"
  echo ""
}

setup_opensource() {
  echo "Initializing repository with default values in Open-Source mode."
  cat "$DEFAULT_PRIVATE_HEADER" > "$PRIVATE_HEADER"
  cat "$DEFAULT_PRIVATE_CAR_MODEL_COEFS" > "$PRIVATE_CAR_MODEL_COEFS"
}


if [ "${1-}" = "-h" -o "${1-}" = "--help" ]; then
  usage
  exit 1
fi

setup_opensource
