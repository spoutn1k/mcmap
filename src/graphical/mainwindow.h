#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QLineEdit>
#include <QMainWindow>
#include <QThread>

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
  QThread renderThread;
  QVector<QWidget *> parameters;
  QVector<QLineEdit *> boundaries;

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void reset_selection();

  void on_renderButton_clicked();

  void on_saveSelectButton_clicked();
  void on_outputSelectButton_clicked();
  void on_colorSelectButton_clicked();
  void on_colorResetButton_clicked();

  void on_orientationNW_toggled(bool);
  void on_orientationSW_toggled(bool);
  void on_orientationSE_toggled(bool);
  void on_orientationNE_toggled(bool);

  void on_dimensionSelectDropDown_currentIndexChanged(int index);

  void on_minX_textEdited(const QString &);
  void on_maxX_textEdited(const QString &);
  void on_minZ_textEdited(const QString &);
  void on_maxZ_textEdited(const QString &);
  void on_minY_textEdited(const QString &);
  void on_maxY_textEdited(const QString &);

  void on_paddingValue_valueChanged(int arg1);

  void on_shading_stateChanged(int);
  void on_lighting_stateChanged(int);

  void startRender();
  void stopRender();
  void updateProgress(int, int, int);

  void on_actionVersion_triggered();

signals:
  void render();

private:
  Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
