# C++ OOP GUI Application Guidelines

## General Principles
- Use modern C++ (C++20/23) with RAII, smart pointers (`std::unique_ptr`, `std::shared_ptr`), and `const` correctness.
- Follow Qt coding standards: CamelCase for classes, camelCase for methods/variables.
- Prefer Qt containers (`QVector`, `QHash`) when interfacing with Qt APIs.
- Use `Q_OBJECT` macro for classes needing signals/slots.
- Apply signals and slots for loose coupling between GUI components.

## Code Structure
- Organize code into MVC-like layers: `ui/`, `logic/`, `data/`.
- Each class has a single responsibility; favor composition over deep inheritance.
- Use forward declarations in headers to reduce compile dependencies.

## Memory & Resources
- Leverage Qt’s parent-child object ownership for automatic GUI element cleanup.
- Use `QPointer` for weak references to `QObject`-derived classes.
- Apply `deleteLater()` for safe deletion of UI objects from other threads.

## UI & Events
- Use `QMainWindow` as the main window base.
- Implement custom widgets by subclassing `QWidget`.
- Use `QTimer` for periodic updates (e.g., sensor polling).
- Handle user input via signals; avoid direct event overrides unless necessary.

## Documentation
- Document classes and public methods with Doxygen-style comments.
- Include `@brief`, `@param`, and `@return` where applicable.

## Example Pattern
```cpp
class SensorWidget : public QWidget {
    Q_OBJECT

public:
    explicit SensorWidget(QWidget *parent = nullptr);
    void updateReading(double value);

signals:
    void readingChanged(double value);

private slots:
    void onRefreshClicked();

private:
    QLabel *m_valueLabel;
    QPushButton *m_refreshButton;
};
```

---

# Project-specific: elcmd (Total Commander prototype)

Build & run (exact commands used in this repo)
- Configure & build (out-of-source): mkdir -p build && cd build && cmake .. && cmake --build . --config Release -j$(nproc)
- Run prototype: ./build/elcmd (from repo root) or ./elcmd from inside build/

Single-test guidance
- No test harness currently present. If tests are added using CTest, run a specific test with: cd build && ctest -R <testname> --output-on-failure

Linting
- No lint configuration present. Recommended quick checks (not required): run clang-tidy via CMake or cpplint on modified files.

High-level architecture (what Copilot should assume)
- UI Layer: MainWindow, PaneWidget (QTableView), Path editors, Button bar (F3..F8).
- Model Layer: FileModel (QAbstractTableModel) exposing Filename/Ext/Size/Date/ATTR and enforcing folder-first sorting.
- Services: FileService (threaded file ops, progress signals), ArchiveService (planned) encapsulate platform differences.

Key repository conventions
- Code lives under src/ — keep headers and sources parallel (Foo.h / Foo.cpp).
- Use Qt model/view for lists (QAbstractTableModel + QTableView). Implement heavy IO in services on background threads.
- Styling: pane background = dark gray; unselected rows bright green; selected rows bright red (use delegate or stylesheet).

Files to inspect for behavior
- src/FileModel.{h,cpp} — listing, sorting logic
- src/PaneWidget.{h,cpp} — view, selection, path editor
- src/FileService.{h,cpp} — file operations
- CMakeLists.txt — how the project builds
- README.md — quick run instructions (kept in sync with this file)

AI assistant integrations
- This file (.github/copilot-instructions.md) is read by Copilot CLI and other assistant agents to prioritize project-specific behavior and shortcuts.
- Also check: AGENTS.md, .github/instructions/, CLAUDE.md, and $HOME/.copilot/copilot-instructions.md when present.

CI and packaging (added):
- A GitHub Actions workflow is included at .github/workflows/ci.yml. It checks out the repo, installs Qt development packages, configures with CMake, builds, runs CTest, and uploads a small artifacts tarball containing the built binary.
- To run CI locally, reproduce the steps:
  1) sudo apt-get update && sudo apt-get install -y build-essential cmake qt6-default qt6-qmake qt6-base-dev qt6-tools-dev
  2) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  3) cmake --build build -- -j$(nproc)
  4) ctest --test-dir build --output-on-failure
- Packaging: CI creates artifacts/elcmd-<sha>.tar.gz with the built binary. For releases, extend CPack or create platform-specific installers.

If anything here is incorrect or you want more detail (single-test commands, CI steps, packaging), tell me which area to expand and I will update this file.
