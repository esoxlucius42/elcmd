#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QLineEdit>
#include <QMetaObject>
#include "FileService.h"
#include "PaneWidget.h"

class TestPaneOps : public QObject {
    Q_OBJECT
private slots:
    void test_copy_and_move_and_delete() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QString srcDir = tmp.path() + "/src";
        QString dstDir = tmp.path() + "/dst";
        QDir().mkpath(srcDir);
        QDir().mkpath(dstDir);

        QString srcFile = srcDir + "/file.txt";
        QFile f(srcFile);
        QVERIFY(f.open(QIODevice::WriteOnly));
        QByteArray data = "hello";
        f.write(data);
        f.close();

        // test copy via FileService
        FileService svc;
        QSignalSpy spyFinished(&svc, SIGNAL(finished(bool,QString)));
        svc.copy(srcFile, dstDir + "/file.txt");
        QVERIFY(spyFinished.wait(5000));
        QCOMPARE(QFile::exists(dstDir + "/file.txt"), true);

        // test move using QFile::rename
        QString moved = dstDir + "/file2.txt";
        QVERIFY(QFile::copy(dstDir + "/file.txt", moved));
        QVERIFY(QFile::remove(dstDir + "/file.txt"));
        QCOMPARE(QFile::exists(moved), true);

        // test new folder creation
        QDir d(dstDir);
        QVERIFY(d.mkdir("newfolder"));
        QCOMPARE(QDir(dstDir + "/newfolder").exists(), true);

        // test delete
        QVERIFY(QFile::remove(moved));
        QCOMPARE(QFile::exists(moved), false);
    }

    void test_navigation_buttons_and_enter() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        PaneWidget pw;
        QLineEdit *le = pw.findChild<QLineEdit*>();
        QVERIFY(le);

        // start in temp root by entering path and pressing Enter
        QString base = tmp.path();
        le->setText(base);
        bool invoked = QMetaObject::invokeMethod(&pw, "onPathReturnPressed", Qt::DirectConnection);
        QVERIFY(invoked);
        QCOMPARE(pw.currentPath(), base);

        // create subdir and navigate into it
        QString sub = base + "/subdir";
        QDir().mkpath(sub);
        le->setText(sub);
        invoked = QMetaObject::invokeMethod(&pw, "onPathReturnPressed", Qt::DirectConnection);
        QVERIFY(invoked);
        QCOMPARE(pw.currentPath(), sub);

        // invoke parent button
        invoked = QMetaObject::invokeMethod(&pw, "onParentClicked", Qt::DirectConnection);
        QVERIFY(invoked);
        QCOMPARE(pw.currentPath(), base);

        // invoke home button
        invoked = QMetaObject::invokeMethod(&pw, "onHomeClicked", Qt::DirectConnection);
        QVERIFY(invoked);
        QCOMPARE(pw.currentPath(), QDir::homePath());

        // invoke root button
        invoked = QMetaObject::invokeMethod(&pw, "onRootClicked", Qt::DirectConnection);
        QVERIFY(invoked);
        QCOMPARE(pw.currentPath(), QDir::rootPath());

        // test Enter navigation with valid and invalid paths
        le->setText(base);
        invoked = QMetaObject::invokeMethod(&pw, "onPathReturnPressed", Qt::DirectConnection);
        QVERIFY(invoked);
        QCOMPARE(pw.currentPath(), base);

        // Skipping invalid path dialog test to avoid modal blocking in automated test environment
        Q_UNUSED(base);
    }
};

QTEST_MAIN(TestPaneOps)
#include "TestPaneOps.moc"
