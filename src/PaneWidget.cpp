#include "PaneWidget.h"
#include <QTableView>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDir>
#include <QMouseEvent>
#include <QItemSelectionModel>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QFile>
#include "FileModel.h"
#include "ArchiveViewerDialog.h"

PaneWidget::PaneWidget(QWidget *parent) : QWidget(parent) {
    m_model = new FileModel(this);
    m_view = new QTableView(this);
    m_view->setModel(m_model);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // basic appearance per spec
    m_view->setStyleSheet("QTableView { background-color: #3a3a3a; color: #00ff00; gridline-color: #222; } QTableView::item:selected { background-color: #7f0000; color: #ffffff; }");

    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setText(QDir::currentPath());

    QVBoxLayout *l = new QVBoxLayout(this);
    l->setContentsMargins(2,2,2,2);
    l->addWidget(m_pathEdit);
    l->addWidget(m_view);

    // install event filter on viewport to customize right-click selection
    m_view->viewport()->installEventFilter(this);

    // connect selection change
    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &PaneWidget::onSelectionChanged);
    connect(m_view, &QTableView::doubleClicked, this, [this](const QModelIndex &idx){
        QString ext = m_model->data(m_model->index(idx.row(), 1)).toString().toLower();
        QString name = m_model->data(m_model->index(idx.row(), 0)).toString();
        QString full = QDir(m_pathEdit->text()).filePath(name);
        if (ext == "zip" || ext == "tar" || ext == "tgz" || ext == "gz") {
            // show archive viewer
            ArchiveViewerDialog dlg(full, this);
            dlg.exec();
        }
    });

    // default path is current working directory
    m_model->setPath(QDir::currentPath());
}

FileModel* PaneWidget::model() const { return m_model; }

QString PaneWidget::currentPath() const { return m_pathEdit->text(); }

QStringList PaneWidget::selectedPaths() const {
    QStringList out;
    QModelIndexList sel = m_view->selectionModel()->selectedRows();
    for (const QModelIndex &idx : sel) {
        QString name = m_model->data(m_model->index(idx.row(), 0)).toString();
        out << QDir(m_pathEdit->text()).filePath(name);
    }
    return out;
}

void PaneWidget::refresh() {
    m_model->setPath(m_pathEdit->text());
}

bool PaneWidget::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_view->viewport() && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::RightButton) {
            QModelIndex idx = m_view->indexAt(me->pos());
            if (idx.isValid()) {
                QItemSelectionModel *sel = m_view->selectionModel();
                if (!sel->isSelected(idx)) {
                    sel->select(idx, QItemSelectionModel::Select | QItemSelectionModel::Rows);
                }

                // build context menu for selected rows
                QString name = m_model->data(m_model->index(idx.row(), 0)).toString();
                QString full = QDir(m_pathEdit->text()).filePath(name);

                QMenu menu(m_view);
                QAction *aView = menu.addAction("View");
                QAction *aEdit = menu.addAction("Edit");
                QAction *aDelete = menu.addAction("Delete");
                QAction *act = menu.exec(m_view->viewport()->mapToGlobal(me->pos()));
                if (act == aView) {
                    // open with default viewer for now
                    QDesktopServices::openUrl(QUrl::fromLocalFile(full));
                    return true;
                } else if (act == aEdit) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(full));
                    return true;
                } else if (act == aDelete) {
                    if (QMessageBox::question(this, "Delete", QString("Delete %1?").arg(name)) == QMessageBox::Yes) {
                        QFile::remove(full);
                        m_model->setPath(m_pathEdit->text());
                    }
                    return true;
                }
                return true; // consumed
            }
        }
        // left click: let default selection behavior occur, but ensure widget gets focus
        if (me->button() == Qt::LeftButton) {
            this->setFocus();
        }
    }
    return QWidget::eventFilter(obj, event);
}

void PaneWidget::onSelectionChanged() {
    QModelIndexList sel = m_view->selectionModel()->selectedRows();
    if (!sel.isEmpty()) {
        QModelIndex idx = sel.last();
        QString name = m_model->data(m_model->index(idx.row(), 0)).toString();
        QString full = QDir(m_pathEdit->text()).filePath(name);
        emit selectionChanged(full);
    }
}

void PaneWidget::setActive(bool active) {
    if (active) {
        m_view->setStyleSheet("QTableView { background-color: #3a3a3a; color: #00ff00; gridline-color: #222; border: 2px solid #888; } QTableView::item:selected { background-color: #7f0000; color: #ffffff; }");
    } else {
        m_view->setStyleSheet("QTableView { background-color: #3a3a3a; color: #00ff00; gridline-color: #222; } QTableView::item:selected { background-color: #7f0000; color: #ffffff; }");
    }
}

void PaneWidget::focusView() {
    m_view->setFocus();
}
