#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QPlainTextEdit>
#include <QScrollBar>

class LogWindow : public QPlainTextEdit {
  Q_OBJECT

public:
  void log(const QString &text);
};

#endif
