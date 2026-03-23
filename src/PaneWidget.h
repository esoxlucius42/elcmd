#pragma once
#include <QWidget>
#include <QSet>

class QTableView;
class QLineEdit;
class QLabel;
class QPushButton;
class FileModel;

class PaneWidget : public QWidget {
    Q_OBJECT
public:
    enum RightClickMode { ContextOnSelected, SelectOnRightClick };
    explicit PaneWidget(RightClickMode mode = SelectOnRightClick, bool useLeftStyling = true, QWidget *parent = nullptr);
    FileModel* model() const;
    void setActive(bool active);
    void focusView();

    // highlight API: view-only single-row highlight (last-interacted item)
    void setHighlightedRow(int row);
    int highlightedRow() const;

    // helper accessors for main app
    QString currentPath() const;
    QStringList selectedPaths() const;
    void refresh();

    // right-click behavior preference
    void setRightClickMode(RightClickMode mode);
    RightClickMode rightClickMode() const;

signals:
    void selectionChanged(const QString &path);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onSelectionChanged();
    void onParentClicked();
    void onHomeClicked();
    void onRootClicked();
    void onPathReturnPressed();

private:
    QLineEdit *m_pathEdit;
    QTableView *m_view;
    FileModel *m_model;
    QPushButton *m_parentBtn;
    QPushButton *m_homeBtn;
    QPushButton *m_rootBtn;
    RightClickMode m_rightClickMode = SelectOnRightClick; // default per spec

    // highlighted row (view-only): index of last-interacted row, -1 none
    int m_highlightedRow = -1;

    // right-click drag/toggle state
    bool m_rightDragActive{false};
    bool m_rightDragWillSelect{false};
    QSet<int> m_rightDragHandledRows;
};
