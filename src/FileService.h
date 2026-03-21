#pragma once
#include <QObject>
#include <QString>
#include <atomic>

class FileService : public QObject {
    Q_OBJECT
public:
    explicit FileService(QObject *parent = nullptr);

    // copy src -> dst, emits progress and finished
    Q_INVOKABLE void copy(const QString &src, const QString &dst);
    Q_INVOKABLE void cancel();

signals:
    void progress(qint64 done, qint64 total);
    void finished(bool success, const QString &message);

private:
    std::atomic<bool> m_cancel{false};
};
