#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QThread>
#include <spdlog/sinks/qt_sinks.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class Renderer : public QObject {
  Q_OBJECT
  QThread renderThread;

public slots:
  void render();

signals:
  void startRender();
  void resultReady();
  void sendProgress(int, int, int);
};

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private:
  QThread renderThread;
  QPlainTextEdit *log_messages;

  std::shared_ptr<spdlog::logger> logger = nullptr;
  void closeEvent(QCloseEvent *);

private slots:
  void on_renderButton_clicked();

  void on_saveSelectButton_clicked();
  void on_singlePNGFileSelect_clicked();
  void on_tiledOutputFileSelect_clicked();
  void on_colorSelectButton_clicked();
  void on_colorResetButton_clicked();

  void on_orientationNW_toggled(bool);
  void on_orientationSW_toggled(bool);
  void on_orientationSE_toggled(bool);
  void on_orientationNE_toggled(bool);

  void on_dimensionSelectDropDown_currentIndexChanged(int index);

  void on_boxMinX_textEdited(const QString &);
  void on_boxMaxX_textEdited(const QString &);
  void on_boxMinZ_textEdited(const QString &);
  void on_boxMaxZ_textEdited(const QString &);
  void on_boxMinY_textEdited(const QString &);
  void on_boxMaxY_textEdited(const QString &);

  void on_circularCenterX_textEdited(const QString &);
  void on_circularCenterZ_textEdited(const QString &);
  void on_circularMinY_textEdited(const QString &);
  void on_circularMaxY_textEdited(const QString &);
  void on_circularRadius_textEdited(const QString &);

  void on_paddingValue_valueChanged(int arg1);

  void on_shading_stateChanged(int);
  void on_lighting_stateChanged(int);

  void startRender();
  void stopRender();
  void updateProgress(int, int, int);

  void on_actionToggleLogs_triggered();
  void on_actionDumpColors_triggered();
  void on_actionExit_triggered();
  void on_actionVersion_triggered();

signals:
  void render();

private:
  Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
