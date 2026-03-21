#include "ArchiveViewerDialog.h"
#include "ArchiveService.h"
#include <QListWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

ArchiveViewerDialog::ArchiveViewerDialog(const QString &archivePath, QWidget *parent)
    : QDialog(parent), m_archivePath(archivePath) {
    setWindowTitle("Archive: " + archivePath);
    m_service = new ArchiveService(this);
    m_list = new QListWidget(this);
    m_extractBtn = new QPushButton("Extract Selected", this);

    QVBoxLayout *l = new QVBoxLayout(this);
    l->addWidget(m_list);
    l->addWidget(m_extractBtn);

    QStringList entries = m_service->listEntries(archivePath);
    m_list->addItems(entries);

    connect(m_extractBtn, &QPushButton::clicked, this, &ArchiveViewerDialog::onExtract);
}

void ArchiveViewerDialog::onExtract() {
    QList<QListWidgetItem*> sel = m_list->selectedItems();
    if (sel.isEmpty()) return;
    QString dest = QFileDialog::getExistingDirectory(this, "Select destination");
    if (dest.isEmpty()) return;
    bool ok = true;
    for (auto it : sel) {
        if (!m_service->extractEntry(m_archivePath, it->text(), dest)) {
            ok = false;
            QMessageBox::warning(this, "Extract failed", "Failed to extract: " + it->text());
        }
    }
    if (ok) QMessageBox::information(this, "Extract", "Extraction complete");
}
