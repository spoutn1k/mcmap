#include "../settings.h"
#include "mainwindow.h"
#include <QApplication>
#include <logger.hpp>

SETUP_LOGGER;

Colors::Palette color_palette;

int main(int argc, char *argv[]) {
  Colors::load(&color_palette);

  QApplication a(argc, argv);
  MainWindow w;
  w.show();
  return a.exec();
}
