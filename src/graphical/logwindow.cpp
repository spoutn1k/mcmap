#include "logwindow.h"

void LogWindow::log(const QString &text) {
  this->appendPlainText(text);
  this->verticalScrollBar()->setValue(this->verticalScrollBar()->maximum());
}
