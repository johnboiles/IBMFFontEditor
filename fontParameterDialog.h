#pragma once

#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>

#include "IBMFDriver/ibmf_defs.hpp"
#include "Unicode/UBlocks.hpp"
#include "freeType.h"

namespace Ui {
class FontParameterDialog;
}

class FontParameterDialog : public QDialog {
  Q_OBJECT

public:
  explicit FontParameterDialog(FreeType &ft, QString title, QWidget *parent = nullptr);
  ~FontParameterDialog();

  IBMFDefs::FontParametersPtr getParameters() { return fontParameters_; }

private slots:
  void browseIBMFFontFilename();
  void browseTTFFontFilename();
  void on_nextButton_clicked();
  void on_cancelButton_clicked();
  void onCheckBoxClicked();

private:
  Ui::FontParameterDialog    *ui;
  IBMFDefs::FontParametersPtr fontParameters_;
  FreeType                   &ft_;

  IBMFDefs::CharSelections *charSelections_;
  QLineEdit                *ibmfFontFilename_;
  QLineEdit                *ttfFontFilename_;
  CodePointBlocks          *codePointBlocks_;
  QRadioButton             *dpi75_;
  QRadioButton             *dpi100_;
  QRadioButton             *dpi120_;
  QCheckBox                *pt12_;
  QCheckBox                *pt14_;
  QCheckBox                *pt17_;

  void checkForNext();
};
