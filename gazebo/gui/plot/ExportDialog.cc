/*
 * Copyright (C) 2016 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/
#include <fstream>
#include <functional>

#include "gazebo/common/SVGLoader.hh"
#include "gazebo/gui/plot/IncrementalPlot.hh"
#include "gazebo/gui/plot/PlotCurve.hh"
#include "gazebo/gui/plot/PlotWindow.hh"
#include "gazebo/gui/plot/ExportDialog.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
/// \brief Private data for the ExportDialog class
class gazebo::gui::ExportDialogPrivate
{
  /// \brief List of the plots that can be exported
  public: QListView *listView;

  /// \brief The export push button
  public: QPushButton *exportButton;
};

/////////////////////////////////////////////////
// \brief Subclass QStandardItem so that we can store a pointer to
/// the plot canvas
class PlotViewItem : public QStandardItem
{
  /// \brief Canvase pointer
  public: PlotCanvas *canvas;
};

/////////////////////////////////////////////////
/// \brief QT delegate used to paint the plot export list view items
class PlotViewDelegate : public QStyledItemDelegate
{
  /// \brief Various data roles for the plot list view delegate
  public: enum datarole
  {
    HEADER_TEXT_ROLE = Qt::UserRole + 100,
    ICON_ROLE = Qt::UserRole
  };

  /// \brief Constructor
  public: PlotViewDelegate() = default;

  /// \brief Destructor
  public: virtual ~PlotViewDelegate() = default;

  /// \brief Paint this item
  /// \param[in] _painter Pointer to the QT painter.
  /// \param[in] _opt Item options.
  /// \param[in] _index Item model index.
  public: void paint(QPainter *_painter, const QStyleOptionViewItem &_opt,
              const QModelIndex &_index) const
          {
            QRectF iconRect = _opt.rect;
            QIcon icon = qvariant_cast<QIcon>(_index.data(ICON_ROLE));
            QString title = qvariant_cast<QString>(
                _index.data(HEADER_TEXT_ROLE));

            QFont font = QApplication::font();
            QFontMetrics fm(font);

            _painter->save();

            iconRect.adjust(10, 10, -10, -fm.height() - 10);

            QPixmap image = icon.pixmap(iconRect.width(), iconRect.height());
            _painter->drawPixmap(iconRect.left(), iconRect.top(), image);

            _painter->setPen(QColor(0, 0, 0));

            QPixmap checkImage;

            if (_opt.state & QStyle::State_Selected)
             checkImage.load(":/images/check_box_black.svg");
            else
             checkImage.load(":/images/check_box_outline_black.svg");

            int checkSize = 24;
            int checkMargin = 4;
            int checkTitleWidth = fm.width(title) + checkSize + checkMargin;

            QRectF checkRect = _opt.rect;
            checkRect.setTop(iconRect.bottom() + checkMargin);
            checkRect.setLeft(iconRect.left() +
                (iconRect.width() - checkTitleWidth)/2.0);
            checkRect.setWidth(checkSize);
            checkRect.setHeight(checkSize);
            _painter->drawPixmap(checkRect, checkImage, checkImage.rect());

            QRectF titleRect = _opt.rect;
            titleRect.setTop(checkRect.top());
            titleRect.setLeft(
                checkRect.left() + checkRect.width() + checkMargin);
            titleRect.setHeight(checkSize);
            _painter->drawText(titleRect, Qt::AlignVCenter, title);
            _painter->restore();
          }

  /// \brief Size hint tells QT how big an item is.
  /// \param[in] _option Style options
  /// \param[in] _index Item model index
  public: QSize sizeHint(const QStyleOptionViewItem &_option,
                         const QModelIndex &_index) const
          {
            QIcon icon = qvariant_cast<QIcon>(_index.data(ICON_ROLE));
            QSize iconSize = icon.actualSize(_option.decorationSize);

            double maxWidth = 320.0;
            double maxHeight = 180.0;

            double ratio = maxWidth / iconSize.width();

            iconSize.scale(maxWidth,  iconSize.height() * ratio,
                Qt::IgnoreAspectRatio);

            if (iconSize.height() > maxHeight)
            {
              ratio = maxHeight / iconSize.height();

              iconSize.scale(iconSize.width() * ratio,  maxHeight,
                  Qt::IgnoreAspectRatio);
            }

            // Add in space at the bottom for the checkbox and text
            QFont font = QApplication::font();
            QFontMetrics fm(font);
            QSize result = QSize(iconSize.width() + 20,
                iconSize.height() + fm.height() + 20);
            return result;
          }

  public: IncrementalPlot *plot;
};

/////////////////////////////////////////////////
ExportDialog::ExportDialog(QWidget *_parent,
    const std::list<PlotCanvas*> &_plots)
: QDialog(_parent),
  dataPtr(new ExportDialogPrivate)
{
  QAction *selectAllAct = new QAction(
      QIcon(":/images/select_all.svg"), tr("Select all"), this);
  selectAllAct->setToolTip(tr("Select all"));
  selectAllAct->setVisible(true);

  QAction *clearAct = new QAction(
      QIcon(":/images/clear.svg"), tr("Clear selection"), this);
  clearAct->setToolTip(tr("Clear selection"));
  clearAct->setVisible(true);

  QToolBar *selectToolbar = new QToolBar;
  selectToolbar->setObjectName("plotToolbar");
  selectToolbar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  selectToolbar->addAction(selectAllAct);
  selectToolbar->addAction(clearAct);

  QHBoxLayout *titleLayout = new QHBoxLayout;
  titleLayout->addWidget(new QLabel("Select plots to export", this));
  titleLayout->addStretch();
  titleLayout->addWidget(selectToolbar);
  titleLayout->setAlignment(Qt::AlignHCenter);
  titleLayout->setContentsMargins(0, 0, 0, 0);

  QFrame *titleFrame = new QFrame;
  titleFrame->setObjectName("plotExportTitleFrame");
  titleFrame->setLayout(titleLayout);

  this->setObjectName("plotExport");
  this->setWindowTitle("Export Plot");
  this->setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint |
                       Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint);

  QHBoxLayout *buttonsLayout = new QHBoxLayout;
  QPushButton *cancelButton = new QPushButton(tr("&Cancel"));
  cancelButton->setObjectName("materialFlat");
  connect(cancelButton, SIGNAL(clicked()), this, SLOT(OnCancel()));

  QMenu *exportMenu = new QMenu;
  exportMenu->setObjectName("material");

  QAction *csvAct = new QAction("CSV (.csv)", exportMenu);
  csvAct->setStatusTip("Export to CSV files");
  connect(csvAct, SIGNAL(triggered()), this, SLOT(OnExportCSV()));

  QAction *pdfAct = new QAction("PDF (.pdf)", exportMenu);
  pdfAct->setStatusTip("Export to PDF files");
  connect(pdfAct, SIGNAL(triggered()), this, SLOT(OnExportPDF()));

  exportMenu->addAction(csvAct);
  exportMenu->addAction(pdfAct);

  this->dataPtr->exportButton = new QPushButton("&Export to");
  this->dataPtr->exportButton->setObjectName("materialFlat");
  this->dataPtr->exportButton->setDefault(true);
  this->dataPtr->exportButton->setEnabled(false);
  this->dataPtr->exportButton->setMenu(exportMenu);

  buttonsLayout->addWidget(cancelButton);
  buttonsLayout->addStretch(2);
  buttonsLayout->addWidget(this->dataPtr->exportButton);

  this->dataPtr->listView = new QListView;
  this->dataPtr->listView->setObjectName("plotExportListView");

  QStandardItemModel *model = new QStandardItemModel();

  //PlotWindow *plotWindow = static_cast<PlotWindow*>(_parent);
  //std::list<PlotCanvas*> plots = plotWindow->Plots();

  for (auto &plot : _plots)
  {
    QIcon icon(QPixmap::grabWindow(plot->winId()));
    PlotViewItem *item = new PlotViewItem;
    item->canvas = plot;

    item->setData(QString::fromStdString(plot->Title()),
        PlotViewDelegate::HEADER_TEXT_ROLE);
    item->setData(icon, PlotViewDelegate::ICON_ROLE);
    item->setEditable(false);
    item->setCheckable(true);
    model->appendRow(item);
  }

  this->dataPtr->listView->setViewMode(QListView::IconMode);
  this->dataPtr->listView->setWrapping(true);
  this->dataPtr->listView->setFlow(QListView::LeftToRight);
  this->dataPtr->listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->dataPtr->listView->setResizeMode(QListView::Adjust);
  this->dataPtr->listView->setMovement(QListView::Static);
  this->dataPtr->listView->setSelectionMode(QAbstractItemView::MultiSelection);
  connect(this->dataPtr->listView, SIGNAL(clicked(const QModelIndex &)),
          this, SLOT(OnSelected()));

  PlotViewDelegate *plotViewDelegate = new PlotViewDelegate;

  this->dataPtr->listView->setModel(model);
  this->dataPtr->listView->setItemDelegate(plotViewDelegate);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(titleFrame);
  mainLayout->addWidget(this->dataPtr->listView);
  mainLayout->addLayout(buttonsLayout);
  mainLayout->addSpacing(20);
  mainLayout->setContentsMargins(0, 0, 0, 0);

  this->setLayout(mainLayout);

  // Set a reasonable default size.
  this->resize(720, 520);

  connect(clearAct, SIGNAL(triggered()),
          this->dataPtr->listView, SLOT(clearSelection()));
  connect(clearAct, SIGNAL(triggered()),
          this, SLOT(OnSelected()));

  connect(selectAllAct, SIGNAL(triggered()),
          this->dataPtr->listView, SLOT(selectAll()));
  connect(selectAllAct, SIGNAL(triggered()),
          this, SLOT(OnSelected()));
}

/////////////////////////////////////////////////
void ExportDialog::OnSelected()
{
  this->dataPtr->exportButton->setEnabled(
      this->dataPtr->listView->selectionModel()->selectedIndexes().size() > 0);
}

/////////////////////////////////////////////////
ExportDialog::~ExportDialog()
{
}

/////////////////////////////////////////////////
void ExportDialog::OnCancel()
{
  this->close();
}

/////////////////////////////////////////////////
void ExportDialog::OnExportPDF()
{
  QFileDialog fileDialog(this, tr("Save Directory"), QDir::homePath());
  fileDialog.setObjectName("material");
  fileDialog.setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint |
      Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint);
  fileDialog.setAcceptMode(QFileDialog::AcceptSave);
  fileDialog.setFileMode(QFileDialog::DirectoryOnly);

  if (fileDialog.exec() != QDialog::Accepted)
    return;

  QStringList selected = fileDialog.selectedFiles();

  if (selected.empty())
    return;

  // Get the selected directory
  std::string dir = selected[0].toStdString();

  QModelIndexList selectedPlots =
    this->dataPtr->listView->selectionModel()->selectedIndexes();

  // Export each selected plot
  for (auto iter = selectedPlots.begin(); iter != selectedPlots.end(); ++iter)
  {
    PlotViewItem *plotItem =
      static_cast<PlotViewItem*>(
          static_cast<QStandardItemModel*>(
            this->dataPtr->listView->model())->itemFromIndex(*iter));

    if (plotItem)
    {
      std::string title = plotItem->canvas->Title();

      // Render the plot to a PDF
      int index = 0;
      std::vector<IncrementalPlot *> plots = plotItem->canvas->Plots();
      for (const auto &plot : plots)
      {
        std::string filename = dir + "/" + title;
        filename += plots.size() > 1 ? std::to_string(index) : "";
        filename += ".pdf";

        QwtPlotRenderer renderer;
        renderer.renderDocument(plot, QString(filename.c_str()),
            QSizeF(280, 216));
        index++;
      }
    }
    else
    {
      gzerr << "Invalid plot item.\n";
    }
  }

  this->close();
}

/////////////////////////////////////////////////
void ExportDialog::OnExportCSV()
{
  QFileDialog fileDialog(this, tr("Save Directory"), QDir::homePath());
  fileDialog.setObjectName("material");
  fileDialog.setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint |
      Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint);
  fileDialog.setAcceptMode(QFileDialog::AcceptSave);
  fileDialog.setFileMode(QFileDialog::DirectoryOnly);

  if (fileDialog.exec() != QDialog::Accepted)
    return;

  // Get the selected directory
  QStringList selected = fileDialog.selectedFiles();

  if (selected.empty())
    return;

  std::string dir = selected[0].toStdString();

  QModelIndexList selectedPlots =
    this->dataPtr->listView->selectionModel()->selectedIndexes();

  // Export each selected plot
  for (auto iter = selectedPlots.begin(); iter != selectedPlots.end(); ++iter)
  {
    PlotViewItem *plotItem =
      static_cast<PlotViewItem*>(
          static_cast<QStandardItemModel*>(
            this->dataPtr->listView->model())->itemFromIndex(*iter));

    if (plotItem)
    {
      std::string title = plotItem->canvas->Title();

      // Save data from each curve into a separate file.
      for (const auto &plot : plotItem->canvas->Plots())
      {
        for (const auto &curve : plot->Curves())
        {
          auto c = curve.lock();
          if (!c)
            continue;

          // Cleanup the title
          std::replace(title.begin(), title.end(), '/', '_');
          std::replace(title.begin(), title.end(), '?', ':');

          // Cleanup the label
          std::string label = c->Label();
          std::replace(label.begin(), label.end(), '/', '_');
          std::replace(label.begin(), label.end(), '?', ':');

          std::string filename = dir + "/" + title + "-" + label + ".csv";

          std::ofstream out(filename);
          out << "x, " << c->Label() << std::endl;
          for (unsigned int j = 0; j < c->Size(); ++j)
          {
            ignition::math::Vector2d pt = c->Point(j);
            out << pt.X() << ", " << pt.Y() << std::endl;
          }
          out.close();
        }
      }
    }
    else
    {
      gzerr << "Invalid plot item.\n";
    }
  }
  this->close();
}
