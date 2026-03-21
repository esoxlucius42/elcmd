#pragma once
#include <QWidget>

class QTableView;
class QLineEdit;
class QLabel;
class FileModel;

class PaneWidget : public QWidget {
    Q_OBJECT
public:
    explicit PaneWidget(QWidget *parent = nullptr);
    FileModel* model() const;
    void setActive(bool active);
    void focusView();

    // helper accessors for main app
    QString currentPath() const;
    QStringList selectedPaths() const;
    void refresh();

    // right-click behavior preference
    enum RightClickMode { ContextOnSelected, SelectOnRightClick };
    void setRightClickMode(RightClickMode mode);
    RightClickMode rightClickMode() const;

signals:
    void selectionChanged(const QString &path);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onSelectionChanged();

private:
    QLineEdit *m_pathEdit;
    QTableView *m_view;
    FileModel *m_model;
    RightClickMode m_rightClickMode = SelectOnRightClick; // default per spec
};
