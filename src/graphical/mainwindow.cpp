#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>

#include "../mcmap.h"

Settings::WorldOptions options;
Map::Orientation selected_orientation;
extern Colors::Palette default_palette;
Colors::Palette custom_palette, file_colors;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  this->setWindowTitle(mcmap::version().c_str());
  statusBar()->showMessage("", 1);

  for (auto element : QVector<QWidget *>({ui->saveSelectButton,
                                          ui->boxMaxX,
                                          ui->boxMaxY,
                                          ui->boxMaxZ,
                                          ui->boxMinX,
                                          ui->boxMinY,
                                          ui->boxMinZ,
                                          ui->circularCenterX,
                                          ui->circularCenterZ,
                                          ui->circularMinY,
                                          ui->circularMaxY,
                                          ui->circularRadius,
                                          ui->coordinatesSelect,
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

  ui->dimensionSelectDropDown->setEnabled(false);
  ui->progressBar->setEnabled(false);
  ui->renderButton->setEnabled(false);
  ui->paddingValue->setValue(Settings::PADDING_DEFAULT);

  for (auto element : QVector<QLineEdit *>(
           {ui->boxMaxX, ui->boxMinX, ui->boxMaxY, ui->boxMinY, ui->boxMaxZ,
            ui->boxMinZ, ui->circularCenterX, ui->circularCenterZ,
            ui->circularMinY, ui->circularMaxY, ui->circularRadius})) {
    element->setEnabled(false);
    boundaries.append(element);
  }

  // Log window setup - might need to be moved to a cleaner spot
  log_messages = new QPlainTextEdit();
  const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  const QFontMetrics fm(fixedFont);

  log_messages->setReadOnly(true);
  log_messages->setFont(fixedFont);
  log_messages->resize(fm.averageCharWidth() * 80, fm.height() * 24);

  // Logger setup
  logger = spdlog::qt_logger_mt("gui", log_messages);
  logger->set_level(spdlog::level::debug);
  spdlog::set_default_logger(logger);
  spdlog::set_pattern("[%l] %v");

  // Create a thread to run mcmap_core in - and get updates
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
  log_messages->close();
  delete log_messages;
  delete ui;
  renderThread.quit();
  renderThread.wait();
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
    logger::error("Parsing file failed: {}", err.what());
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

  ui->boxMinX->setValidator(horizontal);
  ui->boxMaxX->setValidator(horizontal);
  ui->boxMinZ->setValidator(horizontal);
  ui->boxMaxZ->setValidator(horizontal);

  ui->boxMinY->setValidator(vertical);
  ui->boxMaxY->setValidator(vertical);

  ui->circularCenterX->setValidator(horizontal);
  ui->circularCenterZ->setValidator(horizontal);
  ui->circularMinY->setValidator(vertical);
  ui->circularMaxY->setValidator(vertical);
  ui->circularRadius->setValidator(
      new QIntValidator(0, std::numeric_limits<int>::max()));

  ui->boxMinX->setText(std::to_string(options.boundaries.minX).c_str());
  ui->boxMaxX->setText(std::to_string(options.boundaries.maxX).c_str());
  ui->boxMinZ->setText(std::to_string(options.boundaries.minZ).c_str());
  ui->boxMaxZ->setText(std::to_string(options.boundaries.maxZ).c_str());
  ui->boxMinY->setText(std::to_string(options.boundaries.minY).c_str());
  ui->boxMaxY->setText(std::to_string(options.boundaries.maxY).c_str());

  ui->circularMinY->setText(std::to_string(options.boundaries.minY).c_str());
  ui->circularMaxY->setText(std::to_string(options.boundaries.maxY).c_str());
  ui->circularCenterX->setText(std::to_string(0).c_str());
  ui->circularCenterZ->setText(std::to_string(0).c_str());
  ui->circularRadius->setText(std::to_string(0).c_str());

  ui->renderButton->setEnabled(true);
}

void MainWindow::on_boxMinX_textEdited(const QString &) {}

void MainWindow::on_boxMaxX_textEdited(const QString &) {}

void MainWindow::on_boxMinZ_textEdited(const QString &) {}

void MainWindow::on_boxMaxZ_textEdited(const QString &) {}

void MainWindow::on_boxMinY_textEdited(const QString &) {}

void MainWindow::on_boxMaxY_textEdited(const QString &) {}

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

  if (ui->coordinatesSelect->currentWidget() == ui->circularCoordinates) {
    options.boundaries = World::Coordinates();

    options.boundaries.cenX =
        std::stol(ui->circularCenterX->displayText().toStdString());
    options.boundaries.cenZ =
        std::stol(ui->circularCenterZ->displayText().toStdString());

    long radius = std::stol(ui->circularRadius->displayText().toStdString());

    options.boundaries.radius = radius;

    options.boundaries.minY =
        std::stol(ui->circularMinY->displayText().toStdString());
    options.boundaries.maxY =
        std::stol(ui->circularMaxY->displayText().toStdString());

    int paddedRadius = 1.2 * options.boundaries.radius;

    options.boundaries.minX = options.boundaries.cenX - paddedRadius;
    options.boundaries.maxX = options.boundaries.cenX + paddedRadius;
    options.boundaries.minZ = options.boundaries.cenZ - paddedRadius;
    options.boundaries.maxZ = options.boundaries.cenZ + paddedRadius;

    // We use the squared radius many times later; calculate it once here.
    options.boundaries.rsqrd =
        options.boundaries.radius * options.boundaries.radius;
  } else {
    options.boundaries = World::Coordinates();

    options.boundaries.minX =
        std::stol(ui->boxMinX->displayText().toStdString());
    options.boundaries.maxX =
        std::stol(ui->boxMaxX->displayText().toStdString());
    options.boundaries.minZ =
        std::stol(ui->boxMinZ->displayText().toStdString());
    options.boundaries.maxZ =
        std::stol(ui->boxMaxZ->displayText().toStdString());
    options.boundaries.minY =
        std::stol(ui->boxMinY->displayText().toStdString());
    options.boundaries.maxY =
        std::stol(ui->boxMaxY->displayText().toStdString());
  }

  options.boundaries.orientation = selected_orientation;

  emit render();
}

void MainWindow::on_orientationNW_toggled(bool checked) {
  if (checked)
    selected_orientation = Map::NW;
}

void MainWindow::on_orientationSW_toggled(bool checked) {
  if (checked)
    selected_orientation = Map::SW;
}

void MainWindow::on_orientationSE_toggled(bool checked) {
  if (checked)
    selected_orientation = Map::SE;
}

void MainWindow::on_orientationNE_toggled(bool checked) {
  if (checked)
    selected_orientation = Map::NE;
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

void MainWindow::on_actionToggleLogs_triggered() {
  if (!this->log_messages->isVisible()) {
    this->log_messages->show();
  } else {
    this->log_messages->close();
  }
}

void MainWindow::on_actionExit_triggered() { this->close(); }

void MainWindow::closeEvent(QCloseEvent *event) {
  this->log_messages->close();
  event->accept();
}

void MainWindow::on_actionVersion_triggered() {
  QMessageBox::about(
      this, mcmap::version().c_str(),
      fmt::format("{}, {}", mcmap::compilation_options(), COMMENT).c_str());
}
