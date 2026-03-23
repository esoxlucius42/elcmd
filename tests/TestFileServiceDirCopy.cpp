#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include "FileService.h"

class TestFileServiceDirCopy : public QObject {
    Q_OBJECT
private slots:
    void test_copy_directory_success() {
        QTemporaryDir tmpSrc;
        QTemporaryDir tmpDst;
        QVERIFY(tmpSrc.isValid());
        QVERIFY(tmpDst.isValid());
        QString srcDir = tmpSrc.path() + "/sourcedir";
        QDir().mkpath(srcDir);
        // create files
        QFile f1(QDir(srcDir).filePath("a.txt"));
        QVERIFY(f1.open(QIODevice::WriteOnly));
        QByteArray d1 = "hello";
        f1.write(d1);
        f1.close();
        QString sub = srcDir + "/sub";
        QDir().mkpath(sub);
        QFile f2(QDir(sub).filePath("b.txt"));
        QVERIFY(f2.open(QIODevice::WriteOnly));
        QByteArray d2 = "world";
        f2.write(d2);
        f2.close();

        QString dst = QDir(tmpDst.path()).filePath(QFileInfo(srcDir).fileName());

        FileService svc;
        QSignalSpy spyFinished(&svc, SIGNAL(finished(bool,QString)));
        svc.copy(srcDir, dst);
        QVERIFY(spyFinished.wait(5000));
        QCOMPARE(spyFinished.count(), 1);
        QList<QVariant> args = spyFinished.takeFirst();
        bool ok = args.at(0).toBool();
        QCOMPARE(ok, true);
        // verify files
        QVERIFY(QFile::exists(QDir(dst).filePath("a.txt")));
        QFile out1(QDir(dst).filePath("a.txt"));
        QVERIFY(out1.open(QIODevice::ReadOnly));
        QCOMPARE(out1.readAll(), d1);
        QFile out2(QDir(dst).filePath("sub/b.txt"));
        QVERIFY(out2.open(QIODevice::ReadOnly));
        QCOMPARE(out2.readAll(), d2);
    }
};

QTEST_MAIN(TestFileServiceDirCopy)
#include "TestFileServiceDirCopy.moc"
