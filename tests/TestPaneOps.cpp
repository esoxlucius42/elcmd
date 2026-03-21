#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include "FileService.h"

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
};

QTEST_MAIN(TestPaneOps)
#include "TestPaneOps.moc"
