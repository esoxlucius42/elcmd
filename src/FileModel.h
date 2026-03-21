#pragma once
#include <QAbstractTableModel>
#include <QVector>
#include <QDateTime>

struct FileEntry {
    QString name;
    QString ext;
    qint64 size;
    QDateTime mtime;
    QString attr;
    bool is_dir{false};
};

class FileModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column { Name=0, Ext=1, Size=2, Date=3, Attr=4, ColumnCount=5 };
    explicit FileModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override { return ColumnCount; }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // prototype helpers
    void setPath(const QString &path);
private:
    QVector<FileEntry> m_entries;
};
