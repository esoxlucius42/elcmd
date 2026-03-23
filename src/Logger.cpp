#include "Logger.h"
#include <QFile>
#include <QMutex>
#include <QDateTime>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>

struct Logger::Impl {
    QFile file;
    QMutex mutex;
};

Logger &Logger::instance() {
    static Logger inst;
    return inst;
}

Logger::Logger() : m_impl(new Impl()) {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) {
        dir = QDir::homePath() + "/.local/share/elcmd";
    } else {
        dir = dir + "/elcmd";
    }
    QDir d(dir);
    d.mkpath(".");
    QString path = d.filePath("elcmd.log");
    m_impl->file.setFileName(path);
    // open lazily per-write to avoid locking issues across processes; keep it simple
}

Logger::~Logger() {
    delete m_impl;
}

void Logger::log(const QString &level, const QString &message) {
    QMutexLocker locker(&m_impl->mutex);
    QString path = m_impl->file.fileName();
    if (path.isEmpty()) {
        // fallback to default path
        QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (dir.isEmpty()) dir = QDir::homePath() + "/.local/share/elcmd";
        QDir d(dir);
        d.mkpath(".");
        path = d.filePath("elcmd.log");
        m_impl->file.setFileName(path);
    }
    if (!m_impl->file.open(QIODevice::Append | QIODevice::Text)) {
        return; // cannot log
    }
    QTextStream ts(&m_impl->file);
    ts << QDateTime::currentDateTimeUtc().toString(Qt::ISODate) << " [" << level << "] " << message << "\n";
    m_impl->file.close();
}
