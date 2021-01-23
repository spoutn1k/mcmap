#include "../settings.h"
#include "mainwindow.h"
#include <QApplication>
#include <logger.hpp>

SETUP_LOGGER;

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  MainWindow w;
  w.show();
  return a.exec();
}
