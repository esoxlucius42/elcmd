#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QSignalSpy>
#include "FileService.h"

class TestFileService : public QObject {
    Q_OBJECT
private slots:
    void test_copy() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QString src = tmp.path() + "/src.txt";
        QString dst = tmp.path() + "/dst.txt";
        QFile f(src);
        QVERIFY(f.open(QIODevice::WriteOnly));
        QByteArray data = "0123456789";
        f.write(data);
        f.close();

        FileService svc;
        QSignalSpy spyFinished(&svc, SIGNAL(finished(bool,QString)));
        QSignalSpy spyProgress(&svc, SIGNAL(progress(qint64,qint64)));

        svc.copy(src, dst);
        // wait for finished
        QVERIFY(spyFinished.wait(5000));
        QCOMPARE(spyFinished.count(), 1);
        QCOMPARE(QFile::exists(dst), true);
        QFile fd(dst);
        QVERIFY(fd.open(QIODevice::ReadOnly));
        QByteArray read = fd.readAll();
        QCOMPARE(read, data);
    }
};

QTEST_MAIN(TestFileService)
#include "TestFileService.moc"
