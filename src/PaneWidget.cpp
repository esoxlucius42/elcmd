#include "PaneWidget.h"
#include <QTableView>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
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
#include <QPushButton>
#include "FileModel.h"
#include "ArchiveViewerDialog.h"
#include <QTimer>
#include <QColor>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QApplication>

// Delegate that forces bright-green text regardless of style/palette
class BrightTextDelegate : public QStyledItemDelegate {
public:
    BrightTextDelegate(QObject *parent=nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        QString txt = opt.text;
        opt.text.clear();
        // draw background / selection using base implementation (without text)
        QStyledItemDelegate::paint(painter, opt, index);

        painter->save();
        QColor brightGreen(0,255,0);
        painter->setPen(brightGreen);
        QFontMetrics fm(opt.font);
        QString elided = fm.elidedText(txt, Qt::ElideRight, opt.rect.width() - 8);
        QRect r = opt.rect.adjusted(4, 0, -4, 0);
        painter->drawText(r, Qt::AlignVCenter | Qt::AlignLeft, elided);
        painter->restore();
    }
};

PaneWidget::PaneWidget(RightClickMode mode, bool useLeftStyling, QWidget *parent) : QWidget(parent), m_rightClickMode(mode) {
    m_model = new FileModel(this);
    // populate model before creating the view to avoid view reacting to model resets during show
    m_model->setPath(QDir::currentPath());

    m_view = new QTableView(this);
    // avoid header auto-resize during early initialization that may query model size hints
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_view->setModel(m_model);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // basic appearance: use default QTableView visuals. Force bright green text so items are visible on dark backgrounds.
    m_view->setStyleSheet("");
    QPalette pal = m_view->palette();
    QColor brightGreen(0x00, 0xFF, 0x00);
    pal.setColor(QPalette::Text, brightGreen);
    pal.setColor(QPalette::WindowText, brightGreen);
    pal.setColor(QPalette::HighlightedText, brightGreen);
    m_view->setPalette(pal);
    m_view->setAlternatingRowColors(false);
    // hide row numbers
    m_view->verticalHeader()->hide();
    // remove gridlines
    m_view->setShowGrid(false);
    m_view->horizontalHeader()->setStretchLastSection(false);
    m_view->horizontalHeader()->setSectionsClickable(true);
    m_view->setSortingEnabled(true);
    m_view->setAccessibleName("File list pane");
    // use monospaced font for headers and cells
    QFont monoFont("Monospace");
    monoFont.setStyleHint(QFont::TypeWriter);
    m_view->setFont(monoFont);
    m_view->horizontalHeader()->setFont(monoFont);

    // use custom delegate to force bright green text
    m_view->setItemDelegate(new BrightTextDelegate(m_view));

    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setText(QDir::currentPath());
    m_pathEdit->setAccessibleName("Path editor");

    // navigation buttons
    m_parentBtn = new QPushButton("..", this);
    m_homeBtn = new QPushButton("~", this);
    m_rootBtn = new QPushButton("/", this);
    m_parentBtn->setToolTip("Parent folder");
    m_homeBtn->setToolTip("Home folder");
    m_rootBtn->setToolTip("Root folder");
    // size buttons to minimal width for two characters
    {
        QFontMetrics fm(m_parentBtn->font());
        int btnWidth = fm.horizontalAdvance("..") + 16; // small padding
        m_parentBtn->setFixedWidth(btnWidth);
        m_homeBtn->setFixedWidth(btnWidth);
        m_rootBtn->setFixedWidth(btnWidth);
    }

    QHBoxLayout *top = new QHBoxLayout();
    top->setContentsMargins(0,0,0,0);
    top->addWidget(m_pathEdit);
    top->addWidget(m_parentBtn);
    top->addWidget(m_homeBtn);
    top->addWidget(m_rootBtn);

    QVBoxLayout *l = new QVBoxLayout(this);
    l->setContentsMargins(2,2,2,2);
    l->addLayout(top);
    l->addWidget(m_view);

    // install event filter on viewport to customize right-click selection
    m_view->viewport()->installEventFilter(this);

    // connect selection change
    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &PaneWidget::onSelectionChanged);
    connect(m_view, &QTableView::doubleClicked, this, [this](const QModelIndex &idx){
        QString name = m_model->data(m_model->index(idx.row(), 0)).toString();
        QString full = QDir(m_pathEdit->text()).filePath(name);
        QFileInfo fi(full);
        if (!fi.exists()) return;
        if (fi.isDir()) {
            // navigate into directory
            m_pathEdit->setText(fi.absoluteFilePath());
            m_model->setPath(fi.absoluteFilePath());
            return;
        }
        // handle archives specially
        QString ext = fi.suffix().toLower();
        if (ext == "zip" || ext == "tar" || ext == "tgz" || ext == "gz") {
            ArchiveViewerDialog dlg(full, this);
            dlg.exec();
            return;
        }
        // for text-like files or any other file, open with system default
        QDesktopServices::openUrl(QUrl::fromLocalFile(full));
    });

    // navigation handlers
    connect(m_parentBtn, &QPushButton::clicked, this, &PaneWidget::onParentClicked);
    connect(m_homeBtn, &QPushButton::clicked, this, &PaneWidget::onHomeClicked);
    connect(m_rootBtn, &QPushButton::clicked, this, &PaneWidget::onRootClicked);
    connect(m_pathEdit, &QLineEdit::returnPressed, this, &PaneWidget::onPathReturnPressed);


    // column sizing: compute attr width for 5 chars in monospaced font
    QFontMetrics fm(m_view->font());
    int attrWidth = fm.horizontalAdvance("xxxxx") + 8; // five chars + padding

    // find column indices from FileModel enum via header labels
    // assume columns: Name(0), Ext(1), Size(2), Date(3), Attr(4)
    const int nameIdx = 0;
    const int extIdx = 1;
    const int sizeIdx = 2;
    const int dateIdx = 3;
    const int attrIdx = 4;

    m_view->horizontalHeader()->setSectionResizeMode(attrIdx, QHeaderView::Fixed);
    m_view->setColumnWidth(attrIdx, attrWidth);

    // avoid ResizeToContents which queries model size hints during initialization and can trigger crashes in some Qt versions
    m_view->horizontalHeader()->setSectionResizeMode(dateIdx, QHeaderView::Interactive);
    m_view->horizontalHeader()->setSectionResizeMode(nameIdx, QHeaderView::Stretch);
    // keep ext and size reasonable (interactive widths)
    m_view->horizontalHeader()->setSectionResizeMode(extIdx, QHeaderView::Interactive);
    m_view->horizontalHeader()->setSectionResizeMode(sizeIdx, QHeaderView::Interactive);
    // set a sensible default width for small columns
    m_view->setColumnWidth(extIdx, fm.horizontalAdvance("ext") * 3);
    m_view->setColumnWidth(sizeIdx, fm.horizontalAdvance("1,234 KiB") + 12);
}


FileModel* PaneWidget::model() const { return m_model; }

void PaneWidget::setRightClickMode(RightClickMode mode) { m_rightClickMode = mode; }

PaneWidget::RightClickMode PaneWidget::rightClickMode() const { return m_rightClickMode; }

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
    if (obj == m_view->viewport()) {
        // Mouse press handling
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
                    if (me->button() == Qt::RightButton) {
                QModelIndex idx = m_view->indexAt(me->pos());
                if (idx.isValid()) {
                    QItemSelectionModel *sel = m_view->selectionModel();
                    // start a right-button drag toggle operation
                    m_rightDragActive = true;
                    m_rightDragHandledRows.clear();
                    bool initiallySelected = sel->isSelected(idx);
                    m_rightDragWillSelect = !initiallySelected; // if clicked on unselected, we will select
                    // apply to initial row
                    if (m_rightDragWillSelect) sel->select(idx, QItemSelectionModel::Select | QItemSelectionModel::Rows);
                    else sel->select(idx, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
                    m_rightDragHandledRows.insert(idx.row());
                    return true; // consume
                }
            }
            // left click: single-click selects only the clicked row (clear others) unless modifier keys are used
            if (static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                QMouseEvent *lme = static_cast<QMouseEvent*>(event);
                QModelIndex idx = m_view->indexAt(lme->pos());
                this->setFocus();
                // if clicked on a valid row and no modifier keys, clear previous selection and select this row
                if (idx.isValid()) {
                    Qt::KeyboardModifiers mods = lme->modifiers();
                    if (!(mods & (Qt::ControlModifier | Qt::ShiftModifier | Qt::MetaModifier))) {
                        QItemSelectionModel *sel = m_view->selectionModel();
                        sel->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                        // do not consume the event so double-clicks are delivered to the view
                    }
                }
                // otherwise fall through to default behavior (allows modifier-based multi-select)
            }
        }

        // Mouse move handling (for right-button drag selection)
        if (event->type() == QEvent::MouseMove) {
            if (m_rightDragActive) {
                QMouseEvent *me = static_cast<QMouseEvent*>(event);
                QModelIndex idx = m_view->indexAt(me->pos());
                if (idx.isValid() && !m_rightDragHandledRows.contains(idx.row())) {
                    QItemSelectionModel *sel = m_view->selectionModel();
                    if (m_rightDragWillSelect) sel->select(idx, QItemSelectionModel::Select | QItemSelectionModel::Rows);
                    else sel->select(idx, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
                    m_rightDragHandledRows.insert(idx.row());
                }
                return true; // consume while dragging
            }
        }

        // Mouse release handling
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::RightButton && m_rightDragActive) {
                // finish drag
                m_rightDragActive = false;
                m_rightDragHandledRows.clear();
                return true; // consume
            }
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

void PaneWidget::onParentClicked() {
    QDir d(m_pathEdit->text());
    if (!d.exists()) return;
    d.cdUp();
    QString p = d.absolutePath();
    m_pathEdit->setText(p);
    m_model->setPath(p);
}

void PaneWidget::onHomeClicked() {
    QString p = QDir::homePath();
    m_pathEdit->setText(p);
    m_model->setPath(p);
}

void PaneWidget::onRootClicked() {
    QString p = QDir::rootPath();
    m_pathEdit->setText(p);
    m_model->setPath(p);
}

void PaneWidget::onPathReturnPressed() {
    QString path = m_pathEdit->text();
    // expand ~ at start
    if (path.startsWith("~")) {
        path.replace(0, 1, QDir::homePath());
    }
    QString cleaned = QDir::cleanPath(path);
    QFileInfo fi(cleaned);
    if (fi.exists() && fi.isDir()) {
        m_pathEdit->setText(fi.absoluteFilePath());
        m_model->setPath(fi.absoluteFilePath());
    } else {
        QMessageBox::warning(this, "Path not found", QString("Path does not exist or is not a folder:\n%1").arg(path));
    }
}

void PaneWidget::setActive(bool active) {
    // Avoid changing style sheets during initialization — set a property and update later when widget is shown
    if (active) this->setProperty("paneActive", true);
    else this->setProperty("paneActive", false);

}

void PaneWidget::focusView() {
    m_view->setFocus();
}


