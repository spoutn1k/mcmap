#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>

#include "../mcmap.h"

Settings::WorldOptions options;
extern Colors::Palette color_palette;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
}

MainWindow::~MainWindow() { delete ui; }

bool convert(int &dest, const QString &arg, MainWindow *parent) {
  try {
    dest = std::stol(arg.toStdString());
  } catch (const std::exception &err) {
    parent->statusBar()->showMessage(
        parent->tr("Could not convert value to integer"), 2000);
    return false;
  }

  return true;
}

void error(QWidget *widget) {
  QPalette pal = QPalette();

  pal.setColor(QPalette::Window, Qt::red);
  widget->setPalette(pal);
  widget->show();
}

void ok(QWidget *widget) {
  QPalette pal = QPalette();

  pal.setColor(QPalette::Window, Qt::lightGray);
  widget->setPalette(pal);
  widget->show();
}

void MainWindow::reset_selection() {
  ui->dimensionSelectDropDown->setEnabled(false);
  ui->dimensionSelectDropDown->clear();

  ok(ui->minX);
  ok(ui->maxX);
  ok(ui->minZ);
  ok(ui->maxZ);
  ok(ui->minY);
  ok(ui->maxY);

  ui->minX->setText("");
  ui->minX->setEnabled(false);
  ui->minXLabel->setEnabled(false);
  ui->maxX->setText("");
  ui->maxX->setEnabled(false);
  ui->maxXLabel->setEnabled(false);
  ui->minZ->setText("");
  ui->minZ->setEnabled(false);
  ui->minZLabel->setEnabled(false);
  ui->maxZ->setText("");
  ui->maxZ->setEnabled(false);
  ui->maxZLabel->setEnabled(false);
  ui->minY->setText("");
  ui->minY->setEnabled(false);
  ui->minYLabel->setEnabled(false);
  ui->maxY->setText("");
  ui->maxY->setEnabled(false);
  ui->maxYLabel->setEnabled(false);

  ui->renderButton->setEnabled(false);
}

void MainWindow::on_saveSelectButton_clicked() {
  QString filename = QFileDialog::getExistingDirectory(
      this, tr("Open Save Folder"),
      fmt::format("{}/.minecraft/saves", getHome()).c_str());

  if (filename.isEmpty()) {
    statusBar()->showMessage(tr("No directory selected"), 2000);
    return;
  }

  if (!assert_save(filename.toStdString())) {
    statusBar()->showMessage(filename + tr(" is not a save folder"), 2000);
  }

  options.save = SaveFile(filename.toStdString());

  if (options.save.dimensions.empty()) {
    statusBar()->showMessage(tr("No terrain found in ") + filename, 2000);
    options.save = SaveFile();
    return;
  }

  ui->saveNameLabel->setText(options.save.name.c_str());
  statusBar()->showMessage(tr("Selected ") + options.save.name.c_str(), 2000);

  reset_selection();

  for (auto &dim : options.save.dimensions)
    ui->dimensionSelectDropDown->addItem(dim.to_string().c_str());
  ui->dimensionSelectDropDown->setCurrentIndex(-1);

  ui->dimensionSelectDropDown->setEnabled(true);
  ui->dimensionLabel->setEnabled(true);
}

void MainWindow::on_outputSelectButton_clicked() {
  QString filename =
      QFileDialog::getSaveFileName(this, tr("Choose destination"),
                                   getHome().c_str(), tr("PNG file (*.png)"));

  if (filename.isEmpty()) {
    statusBar()->showMessage(tr("No file selected"), 2000);
    return;
  }

  options.outFile = filename.toStdString();
  ui->outputLabel->setText(filename);
}

void MainWindow::on_colorSelectButton_clicked() {
  QString filename = QFileDialog::getOpenFileName(this, tr("Open color file"),
                                                  getHome().c_str(),
                                                  tr("JSON files (*.json)"));

  if (filename.isEmpty()) {
    statusBar()->showMessage(tr("No file selected"), 2000);
    return;
  }

  FILE *f = fopen(filename.toStdString().c_str(), "r");
  Colors::Palette colors_j;

  try {
    colors_j = json::parse(f).get<Colors::Palette>();
  } catch (const std::exception &err) {
    statusBar()->showMessage(
        fmt::format("Parsing file failed: {}\n", err.what()).c_str(), 2000);
    logger::error("Parsing file failed: {}\n", err.what());
    fclose(f);
    return;
  }

  fclose(f);

  ui->colorFileLabel->setText(filename);
}

void MainWindow::on_dimensionSelectDropDown_currentIndexChanged(int index) {
  if (!ui->dimensionSelectDropDown->isEnabled() || index < 0 ||
      size_t(index + 1) > options.save.dimensions.size())
    return;

  options.dim = options.save.dimensions[index];
  statusBar()->showMessage(tr("Scanning ") + options.regionDir().c_str(), 2000);

  Terrain::scanWorldDirectory(options.regionDir(), &options.boundaries);

  ui->minX->setEnabled(true);
  ui->minXLabel->setEnabled(true);
  ui->maxX->setEnabled(true);
  ui->maxXLabel->setEnabled(true);
  ui->minZ->setEnabled(true);
  ui->minZLabel->setEnabled(true);
  ui->maxZ->setEnabled(true);
  ui->maxZLabel->setEnabled(true);
  ui->minY->setEnabled(true);
  ui->minYLabel->setEnabled(true);
  ui->maxY->setEnabled(true);
  ui->maxYLabel->setEnabled(true);

  QValidator *horizontal = new QIntValidator(
      std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), this);
  QValidator *vertical = new QIntValidator(0, 255, this);

  ui->minX->setValidator(horizontal);
  ui->maxX->setValidator(horizontal);
  ui->minZ->setValidator(horizontal);
  ui->maxZ->setValidator(horizontal);

  ui->minY->setValidator(vertical);
  ui->maxY->setValidator(vertical);

  ui->minX->setText(std::to_string(options.boundaries.minX).c_str());
  ui->maxX->setText(std::to_string(options.boundaries.maxX).c_str());
  ui->minZ->setText(std::to_string(options.boundaries.minZ).c_str());
  ui->maxZ->setText(std::to_string(options.boundaries.maxZ).c_str());
  ui->minY->setText(std::to_string(options.boundaries.minY).c_str());
  ui->maxY->setText(std::to_string(options.boundaries.maxY).c_str());

  ui->renderButton->setEnabled(true);
}

void check(QLineEdit *min, QLineEdit *max, int &min_dest, int &max_dest,
           MainWindow *parent) {
  bool min_status = convert(min_dest, min->displayText(), parent);
  bool max_status = convert(max_dest, max->displayText(), parent);

  ok(min);
  ok(max);

  if (min_status)
    ok(min);
  else
    error(min);

  if (max_status)
    ok(max);
  else
    error(max);

  if (min_dest > max_dest) {
    error(min);
    error(max);
  }
}

void MainWindow::on_minX_textEdited(const QString &) {
  check(ui->minX, ui->maxX, options.boundaries.minX, options.boundaries.maxX,
        this);
}

void MainWindow::on_maxX_textEdited(const QString &) {
  check(ui->minX, ui->maxX, options.boundaries.minX, options.boundaries.maxX,
        this);
}

void MainWindow::on_minZ_textEdited(const QString &) {
  check(ui->minZ, ui->maxZ, options.boundaries.minZ, options.boundaries.maxZ,
        this);
}

void MainWindow::on_maxZ_textEdited(const QString &) {
  check(ui->minZ, ui->maxZ, options.boundaries.minZ, options.boundaries.maxZ,
        this);
}

void MainWindow::on_minY_textEdited(const QString &) {
  int min, max;
  check(ui->minY, ui->maxY, min, max, this);

  options.boundaries.minY = min;
  options.boundaries.maxY = max;
}

void MainWindow::on_maxY_textEdited(const QString &) {
  int min, max;
  check(ui->minY, ui->maxY, min, max, this);

  options.boundaries.minY = min;
  options.boundaries.maxY = max;
}

void MainWindow::on_paddingValue_valueChanged(int arg1) {
  options.padding = arg1;
}

void MainWindow::on_renderButton_clicked() {
#ifdef DEBUG
  QMessageBox message;
  message.setText(QString(json(options).dump().c_str()));
  message.exec();
#endif

  mcmap::render(options, color_palette);
}
