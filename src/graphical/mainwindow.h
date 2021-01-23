#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void reset_selection();

  void on_renderButton_clicked();

  void on_saveSelectButton_clicked();
  void on_outputSelectButton_clicked();
  void on_colorSelectButton_clicked();

  void on_dimensionSelectDropDown_currentIndexChanged(int index);

  void on_minX_textEdited(const QString &);
  void on_maxX_textEdited(const QString &);
  void on_minZ_textEdited(const QString &);
  void on_maxZ_textEdited(const QString &);
  void on_minY_textEdited(const QString &);
  void on_maxY_textEdited(const QString &);

  void on_paddingValue_valueChanged(int arg1);

  void on_shading_stateChanged(int);
  void on_hideWater_stateChanged(int);
  void on_hideBeacons_stateChanged(int);

private:
  Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
