#pragma once
#include <QDialog>
#include <QString>
#include <QStringList>

class QListWidget;
class QPushButton;
class ArchiveService;

class ArchiveViewerDialog : public QDialog {
    Q_OBJECT
public:
    explicit ArchiveViewerDialog(const QString &archivePath, QWidget *parent = nullptr);
private slots:
    void onExtract();
private:
    QString m_archivePath;
    QListWidget *m_list;
    QPushButton *m_extractBtn;
    ArchiveService *m_service;
};
