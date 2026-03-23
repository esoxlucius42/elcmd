#include "FileService.h"
#include "Logger.h"
#include <QFile>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrent>
#include <QFileInfo>
#include <QDir>

FileService::FileService(QObject *parent) : QObject(parent) {}

void FileService::copy(const QString &src, const QString &dst) {
    m_cancel.store(false);
    // run in background
    QtConcurrent::run([this, src, dst]() {
        QFileInfo sfi(src);
        if (!sfi.exists()) {
            Logger::instance().log("ERROR", QString("Copy failed: source does not exist: %1").arg(src));
            emit finished(false, "Source does not exist");
            return;
        }
        if (sfi.isDir()) {
            Logger::instance().log("ERROR", QString("Copy failed: source is a directory: %1").arg(src));
            emit finished(false, "Source is a directory");
            return;
        }

        QFile in(src);
        if (!in.open(QIODevice::ReadOnly)) {
            Logger::instance().log("ERROR", QString("Failed to open source %1: %2").arg(src).arg(in.errorString()));
            emit finished(false, "Failed to open source");
            return;
        }
        QString target = dst;
        if (QFile::exists(dst)) {
            // conflict handling: for prototype, choose to rename by appending .copy
            target = dst + ".copy";
        }
        QFile out(target);
        if (!out.open(QIODevice::WriteOnly)) {
            Logger::instance().log("ERROR", QString("Failed to open destination %1: %2").arg(target).arg(out.errorString()));
            in.close();
            emit finished(false, "Failed to open destination");
            return;
        }
        qint64 total = in.size();
        qint64 copied = 0;
        const qint64 bufSize = 1<<20; // 1MB
        while (!in.atEnd()) {
            if (m_cancel.load()) {
                out.close();
                in.close();
                Logger::instance().log("INFO", QString("Copy cancelled: %1 -> %2").arg(src).arg(target));
                emit finished(false, "Cancelled");
                return;
            }
            QByteArray chunk = in.read(bufSize);
            qint64 wrote = out.write(chunk);
            if (wrote == -1) {
                Logger::instance().log("ERROR", QString("Write error while copying to %1: %2").arg(target).arg(out.errorString()));
                out.close();
                in.close();
                emit finished(false, "Write error during copy");
                return;
            }
            copied += wrote;
            emit progress(copied, total);
        }
        out.close();
        in.close();
        Logger::instance().log("INFO", QString("Copy succeeded: %1 -> %2").arg(src).arg(target));
        emit finished(true, "OK");
    });
}

void FileService::cancel() {
    m_cancel.store(true);
}
