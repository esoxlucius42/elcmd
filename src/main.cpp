#include <QApplication>
#include <QMainWindow>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QProgressDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QKeySequence>
#include <QFile>
#include <QLineEdit>
#include <QLabel>
#include <QTextEdit>
#include <QDialog>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QInputDialog>
#include <QDir>
#include <QInputDialog>
#include <QSettings>
#include <QCoreApplication>
#include <QProcess>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QGuiApplication>
#include "PaneWidget.h"
#include "FileService.h"
#include "ConflictDialog.h"
#include "Logger.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);

    // Application identity for QSettings
    QCoreApplication::setOrganizationName("elcmd");
    QCoreApplication::setApplicationName("elcmd");
    QSettings settings;

    QMainWindow win;
    QWidget *central = new QWidget();

    // Helper to detect common sandbox / portal environments and return a short message.
    auto detectSandbox = [&]() -> QString {
        QStringList reasons;
        const QByteArray flatpak = QByteArray::fromRawData(qgetenv("FLATPAK_ID"), qstrlen(qgetenv("FLATPAK_ID")));
        const QByteArray snap = QByteArray::fromRawData(qgetenv("SNAP"), qstrlen(qgetenv("SNAP")));
        const QByteArray portal = QByteArray::fromRawData(qgetenv("XDG_DESKTOP_PORTAL"), qstrlen(qgetenv("XDG_DESKTOP_PORTAL")));
        const QByteArray portalPid = QByteArray::fromRawData(qgetenv("XDG_DESKTOP_PORTAL_PID"), qstrlen(qgetenv("XDG_DESKTOP_PORTAL_PID")));
        if (!flatpak.isEmpty()) reasons << QString("Flatpak (FLATPAK_ID=%1)").arg(QString::fromUtf8(flatpak));
        if (!snap.isEmpty()) reasons << QString("Snap (SNAP=%1)").arg(QString::fromUtf8(snap));
        if (!portal.isEmpty() || !portalPid.isEmpty()) reasons << QString("Desktop portal detected");
        // Check for presence of /.flatpak-info as another hint
        if (QFile::exists("/.flatpak-info") || QFile::exists("/run/.flatpak-info")) reasons << "Flatpak runtime detected";
        return reasons.isEmpty() ? QString() : reasons.join("; ");
    };

    // Helper to open files/paths in external apps. If launching is not allowed in
    // this environment, attempt several fallbacks, capture diagnostic output and
    // present helpful guidance (and copy suggested command to clipboard).
    auto openExternally = [&](const QString &path, QWidget *parent) -> bool {
        if (path.isEmpty()) return false;
        QUrl url = QUrl::fromLocalFile(path);
        QSettings localSettings;
        bool preferDetached = localSettings.value("external/preferStartDetached", false).toBool();

        // 1) Qt's desktop services
        if (!preferDetached && QDesktopServices::openUrl(url)) return true;

        QFileInfo fi(path);
        QStringList tried;
        struct TriedResult { QString cmd; QString out; QString err; int exitCode = -1; bool ok = false; };
        QVector<TriedResult> results;

        // Helper to run a command synchronously to capture stderr/stdout for diagnostics
        auto tryCommand = [&](const QString &program, const QStringList &args)->TriedResult{
            TriedResult r;
            r.cmd = program + " " + args.join(' ');
            QProcess p;
            p.start(program, args);
            if (!p.waitForStarted(2000)) {
                r.ok = false; r.err = "failed to start"; return r;
            }
            p.waitForFinished(3000);
            r.exitCode = p.exitCode();
            r.out = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed();
            r.err = QString::fromLocal8Bit(p.readAllStandardError()).trimmed();
            r.ok = (r.exitCode == 0);
            return r;
        };

        // If target itself is executable, try to run it directly (detached or sync)
        if (fi.isExecutable()) {
            if (preferDetached) {
                if (QProcess::startDetached(path)) return true;
                // fallthrough to try a synchronous attempt for diagnostics
            }
            auto r = tryCommand(path, {});
            results.append(r);
            tried << r.cmd;
            if (r.ok) return true;
        }

#ifdef Q_OS_WIN
        if (preferDetached) {
            if (QProcess::startDetached("cmd", {"/c", "start", "", path})) return true;
        }
        results.append( tryCommand("cmd", {"/c","start","",path}) ); tried << "cmd /c start " + path;
#elif defined(Q_OS_MAC)
        if (preferDetached) {
            if (QProcess::startDetached("open", {path})) return true;
        }
        results.append( tryCommand("open", {path}) ); tried << "open " + path;
#else
        // prefer detached if requested
        if (preferDetached) {
            if (QProcess::startDetached("xdg-open", {path})) return true;
            if (QProcess::startDetached("gio", {"open", path})) return true;
        }
        results.append( tryCommand("xdg-open", {path}) ); tried << "xdg-open " + path;
        results.append( tryCommand("gio", {"open", path}) ); tried << "gio open " + path;
        // try passing a file:// URL
        results.append( tryCommand("xdg-open", {QUrl::fromLocalFile(path).toString()}) ); tried << "xdg-open " + QUrl::fromLocalFile(path).toString();
#endif

        // If any attempt succeeded, accept it
        for (const TriedResult &r : results) if (r.ok) return true;

        // Build diagnostic message
        QStringList diag;
        for (const TriedResult &r : results) {
            diag << QString("Command: %1\nExit: %2\nStdout: %3\nStderr: %4\n").arg(r.cmd).arg(r.exitCode).arg(r.out.isEmpty()?"(none)":r.out).arg(r.err.isEmpty()?"(none)":r.err);
        }
        QString sandboxInfo = detectSandbox();
        if (!sandboxInfo.isEmpty()) sandboxInfo = QString("Environment detected: %1\n\n").arg(sandboxInfo);

        // Auto-copy first suggested command to clipboard for user's convenience
        QString suggested;
        if (!tried.isEmpty()) suggested = tried.first();
        if (!suggested.isEmpty()) {
            QClipboard *cb = QGuiApplication::clipboard();
            cb->setText(suggested);
        }

        QString msg = QString("Unable to launch external application in this environment.\n\nOpen the file manually:\n%1\n\nSuggested command (copied to clipboard):\n%2\n\nDiagnostics:\n%3\n%4").arg(path).arg(suggested).arg(diag.join("\n")).arg(sandboxInfo);
        Logger::instance().log("WARN", QString("Failed to open external file: %1; tried: %2").arg(path).arg(tried.join(", ")));
        QMessageBox::information(parent, "Open File", msg);
        return false;
    };

    QHBoxLayout *layout = new QHBoxLayout();

    PaneWidget *left = new PaneWidget(PaneWidget::SelectOnRightClick, true);
    PaneWidget *right = new PaneWidget(PaneWidget::SelectOnRightClick, true);
    layout->addWidget(left);
    layout->addWidget(right);

    // keyboard: TAB to switch panes
    QShortcut *tabSwitch = new QShortcut(QKeySequence(Qt::Key_Tab), &win);

    // track which pane is active and last selected path
    PaneWidget *active = nullptr;
    QString lastPath;

    auto setActive = [&](PaneWidget *p){
        // Do not call PaneWidget::setActive here. Only update the active pointer and give it focus.
        active = p;
        if (active) active->focusView();
    };

    // set initial active pane
    setActive(left);

    QObject::connect(tabSwitch, &QShortcut::activated, [&]() {
        if (active == left) setActive(right);
        else setActive(left);
    });

    // function key shortcuts on buttons

    // bottom selection path label and button bar
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(layout);

    // readonly textbox that displays full path to last selected object
    QLineEdit *lastSelected = new QLineEdit();
    lastSelected->setReadOnly(true);
    mainLayout->addWidget(lastSelected);

    QHBoxLayout *buttonBar = new QHBoxLayout();
    QPushButton *bF3 = new QPushButton("F3 View");
    QPushButton *bF4 = new QPushButton("F4 Edit");
    QPushButton *bF5 = new QPushButton("F5 Copy");
    QPushButton *bF6 = new QPushButton("F6 Move");
    QPushButton *bF7 = new QPushButton("F7 New Folder");
    QPushButton *bF8 = new QPushButton("F8 Delete");
    QPushButton *bF12 = new QPushButton("F12 Log");
    buttonBar->addWidget(bF3);
    buttonBar->addWidget(bF4);
    buttonBar->addWidget(bF5);
    buttonBar->addWidget(bF6);
    buttonBar->addWidget(bF7);
    buttonBar->addWidget(bF8);
    buttonBar->addWidget(bF12);

    mainLayout->addLayout(buttonBar);
    central->setLayout(mainLayout);

    win.setCentralWidget(central);

    // restore saved size (fallback to 1000x800)
    QSize savedSize = settings.value("mainWindow/size", QSize(1000, 800)).toSize();

    win.setMinimumSize(1000, 800);
    win.resize(savedSize);

    // wire F5 -> copy as example
    FileService *svc = new FileService(&win);

    // assign shortcuts to buttons
    bF3->setShortcut(QKeySequence(Qt::Key_F3));
    bF4->setShortcut(QKeySequence(Qt::Key_F4));
    bF5->setShortcut(QKeySequence(Qt::Key_F5));
    bF6->setShortcut(QKeySequence(Qt::Key_F6));
    bF7->setShortcut(QKeySequence(Qt::Key_F7));
    bF8->setShortcut(QKeySequence(Qt::Key_F8));
    bF12->setShortcut(QKeySequence(Qt::Key_F12));

    QObject::connect(left, &PaneWidget::selectionChanged, [&](const QString &path){
        lastSelected->setText(path);
        lastPath = path;
        setActive(left);
    });
    QObject::connect(right, &PaneWidget::selectionChanged, [&](const QString &path){
        lastSelected->setText(path);
        lastPath = path;
        setActive(right);
    });

    // F3: internal viewer (small text files)
    QObject::connect(bF3, &QPushButton::clicked, [&]() {
        if (lastPath.isEmpty()) { QMessageBox::information(nullptr, "View", "No file selected"); return; }
        QFile f(lastPath);
        if (!f.exists()) { QMessageBox::warning(nullptr, "View", "File does not exist"); return; }
        if (f.size() > 5*1024*1024) { // too big for internal viewer
            if (QMessageBox::question(nullptr, "View", "File is large. Open in external viewer?") == QMessageBox::Yes)
                openExternally(lastPath, &win);
            return;
        }
        if (!f.open(QIODevice::ReadOnly)) { QMessageBox::warning(nullptr, "View", "Failed to open file"); return; }
        QByteArray data = f.readAll();
        f.close();
        QDialog dlg(&win);
        dlg.setWindowTitle("View: " + QFileInfo(lastPath).fileName());
        QVBoxLayout *vl = new QVBoxLayout(&dlg);
        QTextEdit *te = new QTextEdit(&dlg);
        te->setPlainText(QString::fromUtf8(data));
        te->setReadOnly(true);
        vl->addWidget(te);
        dlg.resize(600, 400);
        dlg.exec();
    });

    // F4: open with default editor/application
    QObject::connect(bF4, &QPushButton::clicked, [&]() {
        if (lastPath.isEmpty()) { QMessageBox::information(nullptr, "Edit", "No file selected"); return; }
        if (!openExternally(lastPath, &win)) {
            // User has been informed by openExternally; no further action required
        }
    });

    // F5: Copy selected items from active pane to the other pane
    QObject::connect(bF5, &QPushButton::clicked, [&]() {
        PaneWidget *srcPane = active;
        PaneWidget *dstPane = (active == left) ? right : left;
        QStringList items = srcPane->selectedPaths();
        if (items.isEmpty()) { QMessageBox::information(nullptr, "Copy", "No items selected"); return; }
        for (const QString &src : items) {
            QString dst = QDir(dstPane->currentPath()).filePath(QFileInfo(src).fileName());
            // handle conflict
            if (QFile::exists(dst)) {
                ConflictDialog cd(src, dst, &win);
                if (cd.exec() == QDialog::Rejected) continue;
                switch (cd.choice()) {
                    case ConflictDialog::Overwrite: QFile::remove(dst); break;
                    case ConflictDialog::Skip: continue;
                    case ConflictDialog::Rename: dst = dst + ".copy"; break;
                    case ConflictDialog::Cancel: default: continue;
                }
            }
            QProgressDialog pd(QString("Copying %1").arg(QFileInfo(src).fileName()), "Cancel", 0, 100, &win);
            pd.setWindowModality(Qt::WindowModal);
            QObject::connect(&pd, &QProgressDialog::canceled, svc, &FileService::cancel);
            QObject::connect(svc, &FileService::progress, &pd, [&](qint64 done, qint64 total){ if (total>0) pd.setValue(int(100*done/total)); });
            QObject::connect(svc, &FileService::finished, &win, [&](bool ok, const QString &reason){ if (!ok) {
                    QString full = QString("Failed to copy %1: %2\nSee log: %3").arg(QFileInfo(src).fileName()).arg(reason).arg(Logger::instance().logFilePath());
                    QMessageBox::warning(&win, "Copy failed", full);
                } });
            svc->copy(src, dst);
            pd.exec();
        }
        // refresh target pane
        dstPane->refresh();
    });

    // F6: Move (try rename, otherwise copy+remove)
    QObject::connect(bF6, &QPushButton::clicked, [&]() {
        PaneWidget *srcPane = active;
        PaneWidget *dstPane = (active == left) ? right : left;
        QStringList items = srcPane->selectedPaths();
        if (items.isEmpty()) { QMessageBox::information(nullptr, "Move", "No items selected"); return; }
        for (const QString &src : items) {
            QString dst = QDir(dstPane->currentPath()).filePath(QFileInfo(src).fileName());
            if (QFile::exists(dst)) {
                ConflictDialog cd(src, dst, &win);
                if (cd.exec() == QDialog::Rejected) continue;
                switch (cd.choice()) {
                    case ConflictDialog::Overwrite: QFile::remove(dst); break;
                    case ConflictDialog::Skip: continue;
                    case ConflictDialog::Rename: dst = dst + ".moved"; break;
                    case ConflictDialog::Cancel: default: continue;
                }
            }
            if (!QFile::rename(src, dst)) {
                // fallback to copy+remove
                QProgressDialog pd(QString("Moving %1").arg(QFileInfo(src).fileName()), "Cancel", 0, 100, &win);
                pd.setWindowModality(Qt::WindowModal);
                QObject::connect(&pd, &QProgressDialog::canceled, svc, &FileService::cancel);
                QObject::connect(svc, &FileService::progress, &pd, [&](qint64 done, qint64 total){ if (total>0) pd.setValue(int(100*done/total)); });
                QObject::connect(svc, &FileService::finished, &win, [&](bool ok, const QString &reason){ if (!ok) {
                        QString full = QString("Failed to move %1: %2\nSee log: %3").arg(QFileInfo(src).fileName()).arg(reason).arg(Logger::instance().logFilePath());
                        QMessageBox::warning(&win, "Move failed", full);
                    } });
                svc->copy(src, dst);
                pd.exec();
                QFile::remove(src);
            }
        }
        srcPane->refresh();
        dstPane->refresh();
    });

    // F7: New folder in active pane
    QObject::connect(bF7, &QPushButton::clicked, [&]() {
        bool ok = false;
        QString name = QInputDialog::getText(nullptr, "New Folder", "Folder name:", QLineEdit::Normal, "New Folder", &ok);
        if (!ok || name.isEmpty()) return;
        QDir d(active->currentPath());
        if (!d.mkdir(name)) QMessageBox::warning(nullptr, "New Folder", "Failed to create folder");
        active->refresh();
    });

    // F8: Delete selected items in active pane
    QObject::connect(bF8, &QPushButton::clicked, [&]() {
        QStringList items = active->selectedPaths();
        if (items.isEmpty()) { QMessageBox::information(nullptr, "Delete", "No items selected"); return; }
        if (QMessageBox::question(nullptr, "Delete", QString("Delete %1 items?").arg(items.size())) != QMessageBox::Yes) return;
        for (const QString &p : items) {
            QFileInfo fi(p);
            if (fi.isDir()) QDir(p).removeRecursively(); else QFile::remove(p);
        }
        active->refresh();
    });

    // F12: Open application log in default text viewer
    QObject::connect(bF12, &QPushButton::clicked, [&]() {
        QString path = Logger::instance().logFilePath();
        QFileInfo lf(path);
        if (!lf.exists()) {
            QMessageBox::information(&win, "Log", QString("Log file not found:\n%1").arg(path));
            return;
        }
        openExternally(path, &win);
    });

    // Save size on exit
    QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
        QSize toSave = win.isMaximized() ? win.normalGeometry().size() : win.size();
        settings.setValue("mainWindow/size", toSave);
    });

    // Show window
    win.show();

    return app.exec();
}
