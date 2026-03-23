#include "FileService.h"
#include "Logger.h"
#include <QFile>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrent>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>

static bool copyFileStream(const QString &srcFile, const QString &dstFile, std::atomic<bool> &cancel, qint64 &outBytesWritten, qint64 totalForFile, std::function<void(qint64,qint64)> progressCb) {
    QFile in(srcFile);
    if (!in.open(QIODevice::ReadOnly)) return false;
    QFile out(dstFile);
    QDir parentDir = QFileInfo(dstFile).dir();
    parentDir.mkpath(".");
    if (!out.open(QIODevice::WriteOnly)) { in.close(); return false; }
    const qint64 bufSize = 1<<20;
    while (!in.atEnd()) {
        if (cancel.load()) { out.close(); in.close(); return false; }
        QByteArray chunk = in.read(bufSize);
        qint64 wrote = out.write(chunk);
        if (wrote == -1) { out.close(); in.close(); return false; }
        outBytesWritten += wrote;
        if (progressCb) progressCb(outBytesWritten, totalForFile);
    }
    out.close();
    in.close();
    return true;
}

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
            // Recursive directory copy
            QString targetDirPath = dst;
            if (QFile::exists(dst)) {
                targetDirPath = dst + ".copy";
            }
            QDir targetDir;
            if (!targetDir.mkpath(targetDirPath)) {
                Logger::instance().log("ERROR", QString("Failed to create target directory: %1").arg(targetDirPath));
                emit finished(false, "Failed to create destination directory");
                return;
            }
            // gather file list and total size
            qint64 totalSize = 0;
            QStringList files;
            QDirIterator it(src, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString f = it.next();
                QFileInfo fi(f);
                files.append(f);
                totalSize += fi.size();
            }
            qint64 copied = 0;
            QDir srcBase(src);
            for (const QString &filePath : files) {
                if (m_cancel.load()) { Logger::instance().log("INFO", QString("Copy cancelled: %1 -> %2").arg(src).arg(targetDirPath)); emit finished(false, "Cancelled"); return; }
                QString rel = srcBase.relativeFilePath(filePath);
                QString dstPath = QDir(targetDirPath).filePath(rel);
                qint64 writtenForFile = copied; // we want progress to be cumulative
                bool ok = copyFileStream(filePath, dstPath, m_cancel, writtenForFile, totalSize, [&](qint64 done, qint64 total){ emit progress(done, total); });
                if (!ok) {
                    Logger::instance().log("ERROR", QString("Failed copying file %1 to %2").arg(filePath).arg(dstPath));
                    emit finished(false, "Failed during directory copy");
                    return;
                }
                copied = writtenForFile;
                emit progress(copied, totalSize);
            }
            Logger::instance().log("INFO", QString("Directory copy succeeded: %1 -> %2").arg(src).arg(targetDirPath));
            emit finished(true, "OK");
            return;
        }

        // Regular file copy (unchanged behavior)
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
