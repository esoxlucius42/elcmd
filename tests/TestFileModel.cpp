#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include "FileModel.h"

class TestFileModel : public QObject {
    Q_OBJECT
private slots:
    void test_folderFirstSorting() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QDir dir(tmp.path());
        QVERIFY(dir.mkdir("subdir"));
        QFile f1(dir.filePath("a.txt"));
        QVERIFY(f1.open(QIODevice::WriteOnly));
        f1.write("hello");
        f1.close();
        QFile f2(dir.filePath("b.txt"));
        QVERIFY(f2.open(QIODevice::WriteOnly));
        f2.write("world");
        f2.close();

        FileModel model;
        model.setPath(tmp.path());
        QCOMPARE(model.rowCount(), 3);
        // first row should be the directory
        QModelIndex idx0 = model.index(0, FileModel::Size);
        QVariant size0 = model.data(idx0, Qt::DisplayRole);
        QCOMPARE(size0.toString(), QString("<DIR>"));
    }
};

QTEST_MAIN(TestFileModel)
#include "TestFileModel.moc"
