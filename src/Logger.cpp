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
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) base = QDir::homePath() + "/.local/share";
    QDir d(base);
    // ensure directory exists
    d.mkpath("elcmd");
    QString path = d.filePath("elcmd") + "/elcmd.log";
    m_impl->file.setFileName(path);
}

Logger::~Logger() {
    delete m_impl;
}

QString Logger::defaultLogPath() {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) base = QDir::homePath() + "/.local/share";
    QDir d(base);
    d.mkpath("elcmd");
    return d.filePath("elcmd") + "/elcmd.log";
}

QString Logger::logFilePath() const {
    QString path = m_impl->file.fileName();
    if (path.isEmpty()) return defaultLogPath();
    return path;
}

void Logger::log(const QString &level, const QString &message) {
    QMutexLocker locker(&m_impl->mutex);
    QString path = m_impl->file.fileName();
    if (path.isEmpty()) path = defaultLogPath();
    m_impl->file.setFileName(path);
    if (!m_impl->file.open(QIODevice::Append | QIODevice::Text)) {
        return; // cannot log
    }
    QTextStream ts(&m_impl->file);
    ts << QDateTime::currentDateTimeUtc().toString(Qt::ISODate) << " [" << level << "] " << message << "\n";
    m_impl->file.close();
}
