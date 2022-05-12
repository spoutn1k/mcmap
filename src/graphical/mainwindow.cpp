#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>

#include "../mcmap.h"

const std::string WATER_ID = "minecraft:water";
const std::string BEAM_ID = "mcmap:beacon_beam";

#define DEFAULT_DISABLED_OPTIONS                                               \
  QVector<QWidget *>({ui->coordinatesSelect, ui->colorGroup,                   \
                      ui->outputTypeSelect, ui->orientationGroup,              \
                      ui->progressBar, ui->renderButton,                       \
                      ui->dimensionSelectDropDown})

#define PARAMETERS                                                             \
  QVector<QWidget *>({ui->saveSelectButton, ui->coordinatesSelect,             \
                      ui->outputTypeSelect, ui->colorGroup,                    \
                      ui->orientationGroup, ui->renderButton,                  \
                      ui->dimensionSelectDropDown, ui->worldGroup})

Settings::WorldOptions options = Settings::WorldOptions();
World::Coordinates current_dim_bounds = World::Coordinates();
Map::Orientation selected_orientation = Map::NW;
extern Colors::Palette default_palette;
Colors::Palette custom_palette, file_colors;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  // Setup titles and icons
  this->setWindowTitle(mcmap::version().c_str());
  this->setWindowIcon(QIcon(":/icons/grass_block.png"));
  ui->actionVersion->setIcon(QIcon(":/icons/grass_block.png"));
  ui->actionDumpColors->setIcon(QIcon(":/icons/lapis.png"));
  ui->actionExit->setIcon(QIcon(":/icons/lava.png"));
  ui->actionToggleLogs->setIcon(QIcon(":/icons/sprout.png"));

  // Init status bar
  statusBar()->showMessage("", 1);

  // Default values for special fields
  ui->paddingValue->setValue(Settings::PADDING_DEFAULT);
  ui->singlePNGFileName->setText(
      (fs::current_path() / std::string("output.png")).string().c_str());
  ui->tiledOutputFileName->setText(
      (fs::current_path() / std::string("output")).string().c_str());

  for (auto element : DEFAULT_DISABLED_OPTIONS)
    element->setEnabled(false);

  // Validators - allow only correct values
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

  connect(ui->tiledOutputSizeSlider, &QAbstractSlider::valueChanged, this,
          [this](int val) {
            this->ui->tiledOutputSizeValue->setText(
                fmt::format("{}", val * 64).c_str());
          });

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

inline void error(QWidget *widget) {
  QPalette pal = QPalette();

  pal.setColor(QPalette::Window, Qt::red);
  widget->setPalette(pal);
  widget->show();
}

inline void ok(QWidget *widget) {
  QPalette pal = QPalette();

  pal.setColor(QPalette::Window, Qt::lightGray);
  widget->setPalette(pal);
  widget->show();
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
    return;
  }

  options.save = SaveFile(filename.toStdString());

  if (options.save.dimensions.empty()) {
    statusBar()->showMessage(tr("No terrain found in ") + filename, 2000);
    options.save = SaveFile();
    return;
  }

  ui->saveNameLabel->setText(options.save.name.c_str());

  for (auto element : DEFAULT_DISABLED_OPTIONS)
    element->setEnabled(true);

  for (auto &dim : options.save.dimensions)
    ui->dimensionSelectDropDown->addItem(dim.to_string().c_str());

  // Implicit call to `on_dimensionSelectDropDown_currentIndexChanged(0)`
}

void MainWindow::on_singlePNGFileSelect_clicked() {
  QString filename = QFileDialog::getSaveFileName(
      this, tr("Choose destination"),
      QString::fromStdString(getHome().string()), tr("PNG file (*.png)"));

  if (filename.isEmpty()) {
    statusBar()->showMessage(tr("No file selected"), 2000);
    return;
  }

  ui->singlePNGFileName->setText(filename);
}

void MainWindow::on_tiledOutputFileSelect_clicked() {
  QString filename = QFileDialog::getSaveFileName(
      this, tr("Choose destination"),
      fs::path(ui->tiledOutputFileName->text().toStdString())
          .parent_path()
          .c_str());

  if (filename.isEmpty()) {
    statusBar()->showMessage(tr("No directory selected"), 2000);
    return;
  }

  ui->tiledOutputFileName->setText(filename);
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
  logger::info("Scanning {}", options.regionDir().string());

  current_dim_bounds = options.save.getWorld(options.dim);

  ui->boxMinX->setText(std::to_string(current_dim_bounds.minX).c_str());
  ui->boxMaxX->setText(std::to_string(current_dim_bounds.maxX).c_str());
  ui->boxMinZ->setText(std::to_string(current_dim_bounds.minZ).c_str());
  ui->boxMaxZ->setText(std::to_string(current_dim_bounds.maxZ).c_str());
  ui->boxMinY->setText(std::to_string(current_dim_bounds.minY).c_str());
  ui->boxMaxY->setText(std::to_string(current_dim_bounds.maxY).c_str());

  ui->circularMinY->setText(std::to_string(current_dim_bounds.minY).c_str());
  ui->circularMaxY->setText(std::to_string(current_dim_bounds.maxY).c_str());
  ui->circularCenterX->setText(std::to_string(0).c_str());
  ui->circularCenterZ->setText(std::to_string(0).c_str());
  ui->circularRadius->setText(std::to_string(0).c_str());
}

void check_value(MainWindow *w, QWidget *i, QString text,
                 std::function<bool(int)> check, std::string message) {
  int value = std::numeric_limits<int>::min();

  try {
    value = std::stol(text.toStdString());
  } catch (const std::invalid_argument &err) {
    error(i);
  }

  if (!text.length())
    return;

  if (check(value)) {
    error(i);
    w->statusBar()->showMessage(message.c_str(), 2000);
  } else {
    ok(i);
  }
}

auto in_bounds_x = [](int i) {
  return i < current_dim_bounds.minX || i > current_dim_bounds.maxX;
};

auto in_bounds_z = [](int i) {
  return i < current_dim_bounds.minZ || i > current_dim_bounds.maxZ;
};

auto in_bounds_y = [](int i) {
  return i < current_dim_bounds.minY || i > current_dim_bounds.maxY;
};

auto x_bounds_str = []() {
  return fmt::format("Dimension spans from x: {} to x: {}",
                     current_dim_bounds.minX, current_dim_bounds.maxX);
};

auto z_bounds_str = []() {
  return fmt::format("Dimension spans from z: {} to z: {}",
                     current_dim_bounds.minZ, current_dim_bounds.maxZ);
};

auto y_bounds_str = []() {
  return fmt::format("Dimension spans from y: {} to y: {}",
                     current_dim_bounds.minY, current_dim_bounds.maxY);
};

void MainWindow::on_boxMinX_textEdited(const QString &text) {
  check_value(this, ui->boxMinX, text, in_bounds_x, x_bounds_str());
}

void MainWindow::on_boxMaxX_textEdited(const QString &text) {
  check_value(this, ui->boxMaxX, text, in_bounds_x, x_bounds_str());
}

void MainWindow::on_boxMinZ_textEdited(const QString &text) {
  check_value(this, ui->boxMinZ, text, in_bounds_z, z_bounds_str());
}

void MainWindow::on_boxMaxZ_textEdited(const QString &text) {
  check_value(this, ui->boxMaxZ, text, in_bounds_z, z_bounds_str());
}

void MainWindow::on_boxMinY_textEdited(const QString &text) {
  check_value(this, ui->boxMinY, text, in_bounds_y, y_bounds_str());
}

void MainWindow::on_boxMaxY_textEdited(const QString &text) {
  check_value(this, ui->boxMaxY, text, in_bounds_y, y_bounds_str());
}

void MainWindow::on_circularCenterX_textEdited(const QString &text) {
  check_value(this, ui->circularCenterX, text, in_bounds_x, x_bounds_str());
}

void MainWindow::on_circularCenterZ_textEdited(const QString &text) {
  check_value(this, ui->circularCenterZ, text, in_bounds_z, z_bounds_str());
}

void MainWindow::on_circularMinY_textEdited(const QString &text) {
  check_value(this, ui->circularMinY, text, in_bounds_y, y_bounds_str());
}

void MainWindow::on_circularMaxY_textEdited(const QString &text) {
  check_value(this, ui->circularMaxY, text, in_bounds_y, y_bounds_str());
}

void MainWindow::on_circularRadius_textEdited(const QString &) {}

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
  custom_palette.clear();
  custom_palette.merge(Colors::Palette(file_colors));
  custom_palette.merge(Colors::Palette(default_palette));

  if (ui->hideWater->isChecked())
    custom_palette.insert_or_assign(WATER_ID, Colors::Block());

  if (ui->hideBeacons->isChecked())
    custom_palette.insert_or_assign(BEAM_ID, Colors::Block());

  options.boundaries = World::Coordinates();

  try {
    if (ui->coordinatesSelect->currentWidget() == ui->circularCoordinates) {
      options.boundaries.cenX =
          std::stol(ui->circularCenterX->displayText().toStdString());
      options.boundaries.cenZ =
          std::stol(ui->circularCenterZ->displayText().toStdString());
      options.boundaries.radius =
          std::stol(ui->circularRadius->displayText().toStdString());
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
  } catch (const std::invalid_argument &err) {
    QMessageBox::critical(
        this, "Invalid coordinates",
        "Failed to get coordinates data: check the given coordinates");
    return;
  }

  if (!options.boundaries.intersects(current_dim_bounds)) {
    QMessageBox::critical(
        this, "Invalid area selection",
        "The selected area does not overlap with available terrain");
    return;
  }

  options.boundaries.orientation = selected_orientation;

  auto cropped = options.boundaries;
  cropped.crop(current_dim_bounds);

  if (ui->coordinatesSelect->currentWidget() == ui->boxCoordinates) {
    if (!(cropped == options.boundaries)) {
      QMessageBox::warning(
          this, "Automatic coordinates adjustment",
          fmt::format("The coordinates were cropped to match the available "
                      "terrain\nCropped coordinates: {}",
                      cropped.to_string())
              .c_str());
      options.boundaries = cropped;
    }
  }

  if (ui->outputTypeSelect->currentWidget() == ui->singlePNG) {
    options.tile_size = 0;
    options.outFile = ui->singlePNGFileName->text().toStdString();
    options.padding = std::stol(ui->paddingValue->text().toStdString());
  } else {
    options.padding = 0;
    options.outFile = ui->tiledOutputFileName->text().toStdString();
    options.tile_size =
        std::stol(ui->tiledOutputSizeValue->text().toStdString());
  }

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

  for (const auto &element : PARAMETERS)
    element->setEnabled(false);
};

void MainWindow::stopRender() {
  ui->progressBar->setValue(0);
  ui->progressBar->setEnabled(false);
  ui->progressBar->setTextVisible(false);

  for (const auto &element : PARAMETERS)
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

void MainWindow::on_actionDumpColors_triggered() {
  QString color_dest = QFileDialog::getSaveFileName(
      this, "Color dump file", QDir::homePath(), tr("JSON files (*.json)"));

  Colors::Palette colors;
  // Load colors from the text segment
  Colors::load(&colors);

  std::ofstream color_handle(color_dest.toStdString());

  std::string json_data = json(colors).dump(2, ' ');

  color_handle.write(json_data.c_str(), json_data.length());
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
