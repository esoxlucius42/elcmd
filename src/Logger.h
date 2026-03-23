#pragma once
#include <QString>

class Logger {
public:
    static Logger &instance();
    void log(const QString &level, const QString &message);
    QString logFilePath() const; // returns path to current log file
    static QString defaultLogPath();

private:
    Logger();
    ~Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    struct Impl;
    Impl *m_impl;
};
