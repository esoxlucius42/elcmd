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
#include "PaneWidget.h"
#include "FileService.h"
#include "ConflictDialog.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);

    // Application identity for QSettings
    QCoreApplication::setOrganizationName("elcmd");
    QCoreApplication::setApplicationName("elcmd");
    QSettings settings;

    QMainWindow win;
    QWidget *central = new QWidget();
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
        if (active && active != p) active->setActive(false);
        active = p;
        if (active) { active->setActive(true); active->focusView(); }
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
    buttonBar->addWidget(bF3);
    buttonBar->addWidget(bF4);
    buttonBar->addWidget(bF5);
    buttonBar->addWidget(bF6);
    buttonBar->addWidget(bF7);
    buttonBar->addWidget(bF8);

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
                QDesktopServices::openUrl(QUrl::fromLocalFile(lastPath));
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
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(lastPath))) {
            QMessageBox::warning(nullptr, "Edit", "Failed to launch external editor");
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
            QObject::connect(svc, &FileService::progress, [&](qint64 done, qint64 total){ if (total>0) pd.setValue(int(100*done/total)); });
            QObject::connect(svc, &FileService::finished, [&](bool ok, const QString &msg){ if (!ok) QMessageBox::warning(nullptr, "Copy failed", msg); });
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
                QObject::connect(svc, &FileService::progress, [&](qint64 done, qint64 total){ if (total>0) pd.setValue(int(100*done/total)); });
                QObject::connect(svc, &FileService::finished, [&](bool ok, const QString &msg){ if (!ok) QMessageBox::warning(nullptr, "Move failed", msg); });
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

    // Save size on exit
    QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
        QSize toSave = win.isMaximized() ? win.normalGeometry().size() : win.size();
        settings.setValue("mainWindow/size", toSave);
    });

    // Show window
    win.show();

    return app.exec();
}
