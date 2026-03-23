#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QSignalSpy>
#include <QDir>
#include "FileService.h"

class TestFileServiceFailures : public QObject {
    Q_OBJECT
private slots:
    void test_copy_directory() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QString srcDir = tmp.path() + "/sourcedir";
        QDir().mkpath(srcDir);
        QString dst = tmp.path() + "/dstdir";

        FileService svc;
        QSignalSpy spyFinished(&svc, SIGNAL(finished(bool,QString)));
        svc.copy(srcDir, dst);
        QVERIFY(spyFinished.wait(5000));
        QCOMPARE(spyFinished.count(), 1);
        QList<QVariant> args = spyFinished.takeFirst();
        bool ok = args.at(0).toBool();
        QCOMPARE(ok, true);
    }

    void test_copy_permission_denied() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QString src = tmp.path() + "/src.txt";
        QFile f(src);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("hello");
        f.close();

        QString destDir = tmp.path() + "/destdir";
        QDir().mkpath(destDir);
        // remove write permission from destDir
        QVERIFY(QFile::setPermissions(destDir, QFile::ReadOwner | QFile::ExeOwner | QFile::ReadGroup | QFile::ExeGroup | QFile::ReadOther | QFile::ExeOther));

        QString dst = QDir(destDir).filePath("dst.txt");
        FileService svc;
        QSignalSpy spyFinished(&svc, SIGNAL(finished(bool,QString)));
        svc.copy(src, dst);
        QVERIFY(spyFinished.wait(3000));
        QCOMPARE(spyFinished.count(), 1);
        QList<QVariant> args = spyFinished.takeFirst();
        bool ok = args.at(0).toBool();
        QCOMPARE(ok, false);

        // restore permissions so cleanup can remove dir
        QVERIFY(QFile::setPermissions(destDir, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | QFile::ReadGroup | QFile::ExeGroup | QFile::ReadOther | QFile::ExeOther));
    }
};

QTEST_MAIN(TestFileServiceFailures)
#include "TestFileServiceFailures.moc"
