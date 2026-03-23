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
- F4 behavior: Pressing F4 (now labeled "F4 Open") opens the selected file in the system default viewer/application. If the file is larger than 10 MiB, the app will ask for confirmation before launching the external viewer.

External app launches and sandboxing
- If the app cannot launch third-party applications the environment may be sandboxed (Flatpak/Snap) or use a desktop portal that restricts execs.
- To diagnose failures, elcmd captures diagnostic output and suggests a command (copied to clipboard) that the user can run manually.
- If you run the app from the file manager (e.g. Dolphin) and experience: "For security reasons, launching executables is not allowed in this context", try one of:
  - Run the app from a terminal in your build directory: ./build/elcmd
  - Run the binary outside the sandbox (do not use flatpak/snap) during development
  - If packaged as Flatpak, grant appropriate permissions or run the unconfined instance for testing
- The Options menu provides a "Prefer direct external launches (startDetached)" toggle which forces elcmd to attempt direct process starts when opening files.
