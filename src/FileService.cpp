#include "FileService.h"
#include <QFile>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrent>

FileService::FileService(QObject *parent) : QObject(parent) {}

void FileService::copy(const QString &src, const QString &dst) {
    m_cancel.store(false);
    // run in background
    QtConcurrent::run([this, src, dst]() {
        QFile in(src);
        if (!in.open(QIODevice::ReadOnly)) {
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
                emit finished(false, "Cancelled");
                return;
            }
            QByteArray chunk = in.read(bufSize);
            qint64 wrote = out.write(chunk);
            copied += wrote;
            emit progress(copied, total);
        }
        out.close();
        in.close();
        if (m_cancel.load()) {
            emit finished(false, "Cancelled");
            return;
        }
        emit finished(true, "OK");
    });
}

void FileService::cancel() {
    m_cancel.store(true);
}
