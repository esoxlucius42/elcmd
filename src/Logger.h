#pragma once
#include <QString>

class Logger {
public:
    static Logger &instance();
    void log(const QString &level, const QString &message);

private:
    Logger();
    ~Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    struct Impl;
    Impl *m_impl;
};
