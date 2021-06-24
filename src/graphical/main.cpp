#include "../colors.h"
#include "mainwindow.h"
#include <QApplication>
#include <logger.hpp>

SETUP_LOGGER

Colors::Palette default_palette;

int main(int argc, char *argv[]) {
  Colors::load(&default_palette);

  QApplication a(argc, argv);
  MainWindow w;
  w.show();
  return a.exec();
}
