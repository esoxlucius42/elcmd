#include "ArchiveService.h"
#include <QProcess>
#include <QFileInfo>

#ifdef HAVE_LIBARCHIVE
extern "C" {
#include <archive.h>
#include <archive_entry.h>
}
#endif

ArchiveService::ArchiveService(QObject *parent) : QObject(parent) {}

QStringList ArchiveService::listEntries(const QString &archivePath) const {
#ifdef HAVE_LIBARCHIVE
    QStringList out;
    struct archive *a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);
    if (archive_read_open_filename(a, archivePath.toLocal8Bit().constData(), 10240) != ARCHIVE_OK) {
        archive_read_free(a);
        return {};
    }
    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char *path = archive_entry_pathname(entry);
        if (path) out << QString::fromUtf8(path);
        archive_read_data_skip(a);
    }
    archive_read_free(a);
    return out;
#else
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
#endif
}

bool ArchiveService::extractEntry(const QString &archivePath, const QString &entryName, const QString &destDir) const {
#ifdef HAVE_LIBARCHIVE
    struct archive *a = archive_read_new();
    struct archive *ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS);
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);
    bool ok = false;
    if (archive_read_open_filename(a, archivePath.toLocal8Bit().constData(), 10240) != ARCHIVE_OK) {
        archive_read_free(a);
        archive_write_free(ext);
        return false;
    }
    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char *path = archive_entry_pathname(entry);
        if (!path) { archive_read_data_skip(a); continue; }
        if (QString::fromUtf8(path) == entryName) {
            // ensure destination directory exists
            QDir().mkpath(destDir);
            // set extraction path
            archive_entry_set_pathname(entry, QString(destDir + "/" + QFileInfo(entryName).fileName()).toLocal8Bit().constData());
            if (archive_write_header(ext, entry) != ARCHIVE_OK) {
                archive_read_data_skip(a);
                continue;
            }
            const void *buff;
            size_t size;
            la_int64_t offset;
            while (true) {
                int r = archive_read_data_block(a, &buff, &size, &offset);
                if (r == ARCHIVE_EOF) break;
                if (r != ARCHIVE_OK) { ok = false; break; }
                if (archive_write_data_block(ext, buff, size, offset) != ARCHIVE_OK) { ok = false; break; }
            }
            archive_write_finish_entry(ext);
            ok = true;
            break;
        } else {
            archive_read_data_skip(a);
        }
    }
    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);
    return ok;
#else
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
#endif
}
