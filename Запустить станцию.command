#!/bin/zsh
set -e
station_dir="$(cd "$(dirname "$0")" && pwd)"
open "$station_dir/bin/GroundStation.app"
