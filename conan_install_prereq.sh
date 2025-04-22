#!/bin/bash

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <debug|release>"
  exit 1
fi

BUILD_TYPE=$(echo "$1" | tr '[:upper:]' '[:lower:]')

if [[ "$BUILD_TYPE" != "debug" && "$BUILD_TYPE" != "release" ]]; then
  echo "Invalid build type: $BUILD_TYPE"
  echo "Valid types: debug, release"
  exit 1
fi

echo "Selected build type: $BUILD_TYPE"

if ! command -v conan &> /dev/null; then
  echo "Conan not found. Installing via pip..."
  pip install --user conan
  export PATH="$HOME/.local/bin:$PATH"
fi

PROFILE_NAME="default"

if ! conan profile list | grep -q "$PROFILE_NAME"; then
  echo "Creating Conan profile: $PROFILE_NAME"
  conan profile detect --force
fi

echo "Installing dependencies with Conan..."
conan install . \
  --build=missing \
  -s build_type=$(echo "$BUILD_TYPE" | tr '[:lower:]' '[:upper:]') \
  -pr $PROFILE_NAME
