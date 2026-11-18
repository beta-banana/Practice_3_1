#!/bin/zsh
set -euo pipefail
station_dir="$(cd "$(dirname "$0")" && pwd)"
station_qt="${QT_PREFIX:-}"
if [[ -z "$station_qt" ]]; then
  for station_candidate in "$HOME"/Qt/*/macos(N/); do
    if [[ -d "$station_candidate/lib/QtCharts.framework" ]]; then station_qt="$station_candidate"; fi
  done
fi
if [[ -z "$station_qt" ]]; then
  print 'Укажите путь к Qt 6 с модулем Charts: QT_PREFIX=/путь/к/Qt ./Собрать\ и\ проверить.command'
  exit 1
fi
station_cmake="$(command -v cmake || true)"
if [[ -z "$station_cmake" ]]; then station_cmake="$HOME/Qt/Tools/CMake/CMake.app/Contents/bin/cmake"; fi
station_ctest="${station_cmake%/*}/ctest"
# Временный путь без кириллицы исключает проблемы локали qmake/moc.
station_stage="$(mktemp -d /tmp/groundstation-build.XXXXXX)"
mkdir -p "$station_stage/source"
cp -R "$station_dir/src" "$station_dir/assets" "$station_dir/tests" "$station_dir/resources.qrc" "$station_dir/CMakeLists.txt" "$station_stage/source/"
"$station_cmake" -S "$station_stage/source" -B "$station_stage/build" -DCMAKE_PREFIX_PATH="$station_qt" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0
"$station_cmake" --build "$station_stage/build" -j4
"$station_ctest" --test-dir "$station_stage/build" --output-on-failure
mkdir -p "$station_dir/bin" "$station_dir/verification"
ditto "$station_stage/build/GroundStation.app" "$station_dir/bin/GroundStation.app"
"$station_qt/bin/macdeployqt" "$station_dir/bin/GroundStation.app" -always-overwrite -codesign=-
cp "$station_stage/build/verification/"* "$station_dir/verification/"
print "Готово: $station_dir/bin/GroundStation.app"
print "Сборка и журнал тестов: $station_stage/build"
