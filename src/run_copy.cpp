#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include "FileService.h"
#include "Logger.h"
#include <QStandardPaths>

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QString src = "/var/home/esox/dev/cpp/elcmd/build/test_fileservice_autogen";
    QString dstDir = QDir::homePath() + "/Templates";
    QString dst = QDir(dstDir).filePath(QFileInfo(src).fileName());
    if (argc >= 3) {
        src = QString::fromLocal8Bit(argv[1]);
        dst = QString::fromLocal8Bit(argv[2]);
    }
    QDir().mkpath(dstDir);

    FileService svc(nullptr);
    QObject::connect(&svc, &FileService::progress, [&](qint64 done, qint64 total){
        qDebug() << "Progress:" << done << "/" << total;
    });
    QObject::connect(&svc, &FileService::finished, [&](bool ok, const QString &msg){
        qDebug() << "Finished:" << ok << msg;
        if (!ok) Logger::instance().log("UI", QString("Copy reported failure: %1").arg(msg));
        QTimer::singleShot(0, &app, &QCoreApplication::quit);
    });

    QString sp = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    qDebug() << "QStandardPaths::AppDataLocation=" << sp;
    Logger::instance().log("TEST", QString("run_copy starting: %1 -> %2 (apploc=%3)").arg(src).arg(dst).arg(sp));
    qDebug() << "Starting copy:" << src << "->" << dst;
    svc.copy(src, dst);
    return app.exec();
}
