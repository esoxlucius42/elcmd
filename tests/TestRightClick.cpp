#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QApplication>
#include "PaneWidget.h"
#include "FileModel.h"

class TestRightClick : public QObject {
    Q_OBJECT
private slots:
    void test_select_on_rightclick()
    {
        PaneWidget p;
        p.setRightClickMode(PaneWidget::SelectOnRightClick);
        // simulate a directory with a temporary file
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QString dir = tmp.path();
        p.model()->setPath(dir);
        // create a file
        QString fpath = dir + "/a.txt";
        QFile f(fpath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("x");
        f.close();
        // TODO: simulate right-click in view -- this is a basic unit smoke test
        QVERIFY(p.rightClickMode() == PaneWidget::SelectOnRightClick);
    }
};

QTEST_MAIN(TestRightClick)
#include "TestRightClick.moc"
