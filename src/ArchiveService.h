#pragma once
#include <QObject>
#include <QStringList>

class ArchiveService : public QObject {
    Q_OBJECT
public:
    explicit ArchiveService(QObject *parent = nullptr);

    // Synchronously list archive entries (prototype using system tar/unzip)
    QStringList listEntries(const QString &archivePath) const;

    // Extract a single entry to destination directory (returns success)
    bool extractEntry(const QString &archivePath, const QString &entryName, const QString &destDir) const;
};
