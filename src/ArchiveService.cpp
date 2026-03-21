#include "ArchiveService.h"
#include <QProcess>
#include <QFileInfo>

ArchiveService::ArchiveService(QObject *parent) : QObject(parent) {}

QStringList ArchiveService::listEntries(const QString &archivePath) const {
    QFileInfo fi(archivePath);
    QString ext = fi.suffix().toLower();
    QStringList out;

    QProcess p;
    if (ext == "zip") {
        // unzip -Z1 lists filenames, fallback to unzip -l parsing if -Z1 not available
        p.start("unzip", {"-Z1", archivePath});
        if (!p.waitForFinished(3000)) return {};
        QString stdout = QString::fromLocal8Bit(p.readAllStandardOutput());
        out = stdout.split('\n', Qt::SkipEmptyParts);
        return out;
    } else {
        // use tar -tf for tar-based archives
        p.start("tar", {"-tf", archivePath});
        if (!p.waitForFinished(3000)) return {};
        QString stdout = QString::fromLocal8Bit(p.readAllStandardOutput());
        out = stdout.split('\n', Qt::SkipEmptyParts);
        return out;
    }
}

bool ArchiveService::extractEntry(const QString &archivePath, const QString &entryName, const QString &destDir) const {
    QFileInfo fi(archivePath);
    QString ext = fi.suffix().toLower();
    QProcess p;
    if (ext == "zip") {
        // unzip <archive> <entry> -d <destDir>
        p.start("unzip", {archivePath, entryName, "-d", destDir});
        if (!p.waitForFinished(10000)) return false;
        return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
    } else {
        // tar -xf <archive> -C <destDir> <entryName>
        p.start("tar", {"-xf", archivePath, "-C", destDir, entryName});
        if (!p.waitForFinished(10000)) return false;
        return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
    }
}
