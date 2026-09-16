#!/usr/bin/env bash
# Сборка VectorworksLauncherEN.exe (native Win32, без .NET) через mingw-w64 на Ubuntu/Linux.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="build"
mkdir -p "$BUILD_DIR"

WINDRES="${WINDRES:-x86_64-w64-mingw32-windres}"
CXX="${CXX:-x86_64-w64-mingw32-g++}"

if ! command -v "$WINDRES" >/dev/null 2>&1; then
  echo "Не найден $WINDRES. Установите: sudo aptitude install mingw-w64" >&2
  exit 1
fi

if ! command -v "$CXX" >/dev/null 2>&1; then
  echo "Не найден $CXX. Установите: sudo aptitude install mingw-w64 g++-mingw-w64-x86-64" >&2
  exit 1
fi

echo "Компиляция ресурсов (иконка)..."
"$WINDRES" -O coff resources/VectorworksLauncherEN.rc -o "$BUILD_DIR/VectorworksLauncherEN.res" -I.

echo "Компиляция и линковка..."
"$CXX" -std=c++17 -O2 -Wall -Wextra -mwindows -municode -static-libgcc -static-libstdc++ \
  src/main.cpp \
  src/keyboard_layout.cpp \
  src/vectorworks_discovery.cpp \
  src/user_prompts.cpp \
  "$BUILD_DIR/VectorworksLauncherEN.res" \
  -o "$BUILD_DIR/VectorworksLauncherEN.exe" \
  -luser32 -lkernel32 -lcomdlg32 -lshell32

# Копия для каталога на сайте (корень хаба / dist)
DIST_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)/dist"
mkdir -p "$DIST_DIR"
cp -a "$BUILD_DIR/VectorworksLauncherEN.exe" "$DIST_DIR/VectorworksLauncherEN.exe"

echo "Готово: $BUILD_DIR/VectorworksLauncherEN.exe"
echo "Каталог: $DIST_DIR/VectorworksLauncherEN.exe"
