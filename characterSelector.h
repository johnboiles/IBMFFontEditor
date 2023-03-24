#pragma once

#include <QDialog>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

#include "IBMFDriver/ibmf_defs.hpp"

class CharacterSelector : public QDialog {
  Q_OBJECT
public:
  CharacterSelector(const IBMFDefs::CharCodes *chars, QString title = nullptr,
                    QString info = nullptr, QWidget *parent = nullptr);
  int selectedCharIndex() { return _selectedCharIndex; }

private slots:
  void onDoubleClick(const QModelIndex &index);
  void onOk(bool checked = false);
  void onCancel(bool checked = false);
  void onSelected();

private:
  int           _selectedCharIndex;
  int           _columnCount;
  QTableWidget *_charsTable;
  QPushButton  *_okButton;
};
