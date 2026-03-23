#include "FileModel.h"

#include "FileModel.h"
#include <QDir>
#include <QFileInfo>
#include <algorithm>

// Helper: format sizes into a fixed-width 8-character right-aligned string.
// Binary units (KiB = 1024) are used: B, KiB, MiB, GiB, TiB.
static QString formatSize(qint64 size) {
    const char *units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    // Bytes: show integer, and add an extra space before 'B' so numbers align with decimal formats.
    if (size < 1024) {
        QString num = QString::number(size);
        QString s = num + "  " + QString::fromLatin1(units[0]); // two spaces before 'B'
        if (s.length() > 8) return QString("########");
        return s.rightJustified(8, ' ');
    }

    double v = static_cast<double>(size);
    int unit = 0;
    while (v >= 1024.0 && unit < 4) {
        v /= 1024.0;
        ++unit;
    }
    // Format with one decimal place
    QString num = QString::number(v, 'f', 1);
    QString s = num + ' ' + QString::fromLatin1(units[unit]);
    // If too long, try without decimals (integer)
    if (s.length() > 8) {
        QString num2 = QString::number(static_cast<long long>(std::llround(v)));
        s = num2 + ' ' + QString::fromLatin1(units[unit]);
        if (s.length() > 8) return QString("########");
    }
    return s.rightJustified(8, ' ');
}

FileModel::FileModel(QObject *parent) : QAbstractTableModel(parent) {
}

int FileModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return m_entries.size();
}

QVariant FileModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) return {};
    const FileEntry &e = m_entries.at(index.row());
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case Name: return e.name;
            case Ext: return e.ext;
            case Size: return e.is_dir ? QStringLiteral("<DIR>") : formatSize(e.size);
            case Date: return e.mtime.toString(Qt::ISODate);
            case Attr: return e.attr;
        }
    }
    return {};
}

QVariant FileModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case Name: return QStringLiteral("Filename");
            case Ext: return QStringLiteral("Ext");
            case Size: return QStringLiteral("Size");
            case Date: return QStringLiteral("Modified");
            case Attr: return QStringLiteral("ATTR");
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

static bool folderFirstCompare(const FileEntry &a, const FileEntry &b) {
    if (a.is_dir != b.is_dir) return a.is_dir; // folders first
    // folders and files: sort by name
    return a.name.toLower() < b.name.toLower();
}

void FileModel::setPath(const QString &path) {
    beginResetModel();
    m_entries.clear();

    QDir dir(path);
    dir.setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);
    QFileInfoList list = dir.entryInfoList();
    for (const QFileInfo &fi : list) {
        FileEntry e;
        e.name = fi.fileName();
        e.is_dir = fi.isDir();
        e.ext = fi.suffix();
        e.size = fi.isDir() ? 0 : fi.size();
        e.mtime = fi.lastModified();
        // simple rwx attr mapping: owner readable/writable/executable
        e.attr = QString("%1%2%3").arg(fi.isReadable() ? 'r' : '-').arg(fi.isWritable() ? 'w' : '-').arg(fi.isExecutable() ? 'x' : '-');
        m_entries.append(e);
    }
    std::sort(m_entries.begin(), m_entries.end(), folderFirstCompare);

    endResetModel();
}
