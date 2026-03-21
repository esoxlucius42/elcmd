#pragma once
#include <QDialog>

class ConflictDialog : public QDialog {
    Q_OBJECT
public:
    enum Choice { Overwrite, Skip, Rename, Cancel };
    explicit ConflictDialog(const QString &src, const QString &dst, QWidget *parent = nullptr);
    Choice choice() const { return m_choice; }
private:
    Choice m_choice{Cancel};
};
