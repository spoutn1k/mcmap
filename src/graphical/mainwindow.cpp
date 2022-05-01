#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>

#include "../VERSION"
#include "../mcmap.h"

Settings::WorldOptions options;
extern Colors::Palette default_palette;
Colors::Palette custom_palette, file_colors;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  this->setWindowTitle(VERSION);

  for (auto element : QVector<QWidget *>({ui->saveSelectButton,
                                          ui->maxX,
                                          ui->maxY,
                                          ui->maxZ,
                                          ui->minX,
                                          ui->minY,
                                          ui->minZ,
                                          ui->shading,
                                          ui->lighting,
                                          ui->hideBeacons,
                                          ui->hideWater,
                                          ui->paddingValue,
                                          ui->outputSelectButton,
                                          ui->colorSelectButton,
                                          ui->colorResetButton,
                                          ui->orientationNE,
                                          ui->orientationNW,
                                          ui->orientationSE,
                                          ui->orientationSW,
                                          ui->renderButton,
                                          ui->dimensionSelectDropDown}))
    parameters.append(element);

  for (auto element : QVector<QLineEdit *>(
           {ui->maxX, ui->minX, ui->maxY, ui->minY, ui->maxZ, ui->minZ}))
    boundaries.append(element);

  Renderer *renderer = new Renderer;
  renderer->moveToThread(&renderThread);

  connect(&renderThread, SIGNAL(finished()), renderer, SLOT(deleteLater()));
  connect(this, SIGNAL(render()), renderer, SLOT(render()));
  connect(renderer, SIGNAL(startRender()), this, SLOT(startRender()));
  connect(renderer, SIGNAL(resultReady()), this, SLOT(stopRender()));
  connect(renderer, SIGNAL(sendProgress(int, int, int)), this,
          SLOT(updateProgress(int, int, int)));

  renderThread.start();
}

MainWindow::~MainWindow() {
  delete ui;
  renderThread.quit();
  renderThread.wait();
}

bool convert(int &dest, const QString &arg, MainWindow *parent) {
  try {
    dest = std::stol(arg.toStdString());
  } catch (const std::exception &err) {
    parent->statusBar()->showMessage("Could not convert value to integer",
                                     2000);
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

  for (auto element : boundaries) {
    ok(element);
    element->setText("");
    element->setEnabled(false);
  }

  ui->renderButton->setEnabled(false);
}

void MainWindow::on_saveSelectButton_clicked() {
  QString filename = QFileDialog::getExistingDirectory(
      this, tr("Open Save Folder"),
      QString::fromStdString(getSaveDir().string()));

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
  statusBar()->showMessage(
      tr("Selected ") + QString::fromStdString(options.save.name), 2000);

  reset_selection();

  for (auto &dim : options.save.dimensions)
    ui->dimensionSelectDropDown->addItem(dim.to_string().c_str());
  ui->dimensionSelectDropDown->setCurrentIndex(-1);

  ui->dimensionSelectDropDown->setEnabled(true);
  ui->dimensionLabel->setEnabled(true);
}

void MainWindow::on_outputSelectButton_clicked() {
  QString filename = QFileDialog::getSaveFileName(
      this, tr("Choose destination"),
      QString::fromStdString(getHome().string()), tr("PNG file (*.png)"));

  if (filename.isEmpty()) {
    statusBar()->showMessage(tr("No file selected"), 2000);
    return;
  }

  options.outFile = filename.toStdString();
  ui->outputLabel->setText(filename);
}

void MainWindow::on_colorSelectButton_clicked() {
  QString filename = QFileDialog::getOpenFileName(
      this, tr("Open color file"), QString::fromStdString(getHome().string()),
      tr("JSON files (*.json)"));
  if (filename.isEmpty()) {
    statusBar()->showMessage(tr("No file selected"), 2000);
    return;
  }

  FILE *f = fopen(filename.toStdString().c_str(), "r");

  try {
    file_colors = json::parse(f).get<Colors::Palette>();
  } catch (const std::exception &err) {
    statusBar()->showMessage(
        fmt::format("Parsing file failed: {}\n", err.what()).c_str(), 2000);
    logger::error("Parsing file failed: {}\n", err.what());
    fclose(f);
    return;
  }

  fclose(f);

  ui->colorFileLabel->setText(QString::fromStdString(
      fs::path(filename.toStdString()).filename().string()));
}

void MainWindow::on_colorResetButton_clicked() {
  file_colors = Colors::Palette();
  ui->colorFileLabel->setText("Not selected.");
}

void MainWindow::on_dimensionSelectDropDown_currentIndexChanged(int index) {
  if (!ui->dimensionSelectDropDown->isEnabled() || index < 0 ||
      size_t(index + 1) > options.save.dimensions.size())
    return;

  options.dim = options.save.dimensions[index];
  statusBar()->showMessage(
      tr("Scanning ") + QString::fromStdString(options.regionDir().string()),
      2000);

  options.boundaries = options.save.getWorld(options.dim);

  for (auto e : boundaries)
    e->setEnabled(true);

  QValidator *horizontal = new QIntValidator(
      std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), this);
  QValidator *vertical =
      new QIntValidator(mcmap::constants::min_y, mcmap::constants::max_y, this);

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

void MainWindow::on_shading_stateChanged(int checked) {
  options.shading = checked;
}

void MainWindow::on_lighting_stateChanged(int checked) {
  options.lighting = checked;
}

void MainWindow::on_renderButton_clicked() {
  std::string water_id = "minecraft:water";
  std::string beam_id = "mcmap:beacon_beam";

  custom_palette.clear();
  custom_palette.merge(Colors::Palette(file_colors));
  custom_palette.merge(Colors::Palette(default_palette));

  if (ui->hideWater->isChecked())
    custom_palette.insert_or_assign(water_id, Colors::Block());

  if (ui->hideBeacons->isChecked())
    custom_palette.insert_or_assign(beam_id, Colors::Block());

  emit render();
}

void MainWindow::on_orientationNW_toggled(bool checked) {
  if (checked)
    options.boundaries.orientation = Map::NW;
}

void MainWindow::on_orientationSW_toggled(bool checked) {
  if (checked)
    options.boundaries.orientation = Map::SW;
}

void MainWindow::on_orientationSE_toggled(bool checked) {
  if (checked)
    options.boundaries.orientation = Map::SE;
}

void MainWindow::on_orientationNE_toggled(bool checked) {
  if (checked)
    options.boundaries.orientation = Map::NE;
}

void MainWindow::updateProgress(int prog, int total, int action) {
  if (!prog) {
    ui->progressBar->setMaximum(total);

    ui->progressBar->setTextVisible(true);
    ui->progressBar->setFormat(
        QString::fromStdString(
            Progress::action_strings.at(Progress::Action(action))) +
        " [%p%]");
  }

  ui->progressBar->setValue(prog);
}

void MainWindow::startRender() {
  ui->progressBar->setEnabled(true);
  ui->progressBar->setTextVisible(true);

  for (const auto &element : parameters)
    element->setEnabled(false);
};

void MainWindow::stopRender() {
  ui->progressBar->setValue(0);
  ui->progressBar->setEnabled(false);
  ui->progressBar->setTextVisible(false);

  for (const auto &element : parameters)
    element->setEnabled(true);
};

void Renderer::render() {
  auto update = [this](int d, int t, Progress::Action a) {
    emit sendProgress(d, t, a);
  };

  emit startRender();
  mcmap::render(options, custom_palette, update);
  emit resultReady();
}

void MainWindow::on_actionVersion_triggered() {
  QMessageBox::about(this, VERSION, COMMENT);
}
