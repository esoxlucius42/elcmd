Prototype Qt application (Total Commander clone) - skeleton

This repository contains a minimal Qt/C++ prototype for the two-pane file manager used during Sprint 1.

Build (example):
- cmake --build build --target elcmd -- -j4

OLD
- mkdir build && cd build
- cmake ..
- cmake --build .

Run:
- ./elcmd

Notes:
- This is a prototype with stub data in FileModel; file system integration and sorting are next tasks.

Logging:
- Errors and important events are written to a per-user log file at:
  - QStandardPaths::AppDataLocation/elcmd/elcmd.log (preferred)
  - Fallback: ~/.local/share/elcmd/elcmd.log
- When file operations fail, the UI shows a concise message and points to the log file for details.
