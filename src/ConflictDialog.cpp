#include "ConflictDialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

ConflictDialog::ConflictDialog(const QString &src, const QString &dst, QWidget *parent) : QDialog(parent) {
    setWindowTitle("Conflict");
    QVBoxLayout *l = new QVBoxLayout(this);
    l->addWidget(new QLabel("File exists: " + dst + "\nSource: " + src));
    QPushButton *bOverwrite = new QPushButton("Overwrite", this);
    QPushButton *bSkip = new QPushButton("Skip", this);
    QPushButton *bRename = new QPushButton("Rename", this);
    QPushButton *bCancel = new QPushButton("Cancel", this);
    l->addWidget(bOverwrite);
    l->addWidget(bSkip);
    l->addWidget(bRename);
    l->addWidget(bCancel);
    connect(bOverwrite, &QPushButton::clicked, this, [this](){ m_choice = Overwrite; accept(); });
    connect(bSkip, &QPushButton::clicked, this, [this](){ m_choice = Skip; accept(); });
    connect(bRename, &QPushButton::clicked, this, [this](){ m_choice = Rename; accept(); });
    connect(bCancel, &QPushButton::clicked, this, [this](){ m_choice = Cancel; reject(); });
}
