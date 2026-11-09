#!/bin/sh
# Opens the built disc in PCSX-Redux. Set REDUX to the emulator binary if it lives elsewhere.
cd "$(dirname "$0")/.."
exec "${REDUX:-$HOME/ps1-emu/PCSX-Redux.app/Contents/MacOS/PCSX-Redux}" -run -iso build/xenoswell.cue
