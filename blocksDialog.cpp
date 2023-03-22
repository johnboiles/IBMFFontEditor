#include "blocksDialog.h"

#include <iostream>
#include <set>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidget>

#include "Unicode/UBlocks.hpp"
#include "ui_blocksDialog.h"

BlocksDialog::BlocksDialog(QString fontFile, QString fontName, QWidget *parent)
    : QDialog(parent), ui(new Ui::BlocksDialog) {
  ui->setupUi(this);

  this->setWindowTitle("Font Content");
  ui->titleLabel->setText("CodePoint Selection from " + fontName);

  FT_Error error = FT_Err_Ok;
  error          = FT_Init_FreeType(&ftLib_);
  if (error) {
    QMessageBox::warning(this, "Freetype Not Initialized", FT_Error_String(error));
    reject();
  }

  error = FT_New_Face(ftLib_, fontFile.toStdString().c_str(), 0, &ftFace_);
  if (error) {
    QMessageBox::warning(this, "Unable to open font", FT_Error_String(error));
    reject();
  }

  if (ftFace_->charmap == nullptr) {
    QMessageBox::warning(this, "Not Unicode", "There is no Unicode charmap in this font!");
    reject();
  }

  for (int n = 0; n < ftFace_->num_charmaps; n++) {
    std::cout << "PlatformId: " << ftFace_->charmap[n].platform_id
              << " EncodingId: " << ftFace_->charmap[n].encoding_id << std::endl;
  }

  FT_UInt  index;
  char32_t ch = FT_Get_First_Char(ftFace_, &index);

  struct CustomCmp {
    bool operator()(const CodePointBlock *a, const CodePointBlock *b) const {
      return a->blockIdx_ < b->blockIdx_;
    }
  };

  std::set<CodePointBlock *, CustomCmp> blocksSet;

  while (index != 0) {
    int idx;
    //    std::cout << "CodePoint: " << +ch << "(" << ch << ")" << std::endl;
    if ((idx = UnicodeBlocs::findUBloc(ch)) != -1) {
      // std::cout << "block: " << uBlocks[idx].caption_ << std::endl;
      CodePointBlock blk(idx, 1);
      auto           it = blocksSet.find(&blk);
      if (it != blocksSet.end()) {
        (*it)->codePointCount_++;
      } else {
        blocksSet.insert(new CodePointBlock(idx, 1));
      }
    }
    ch = FT_Get_Next_Char(ftFace_, ch, &index);
  }

  blocks_.clear();
  blocks_.reserve(blocksSet.size());

  std::copy(blocksSet.begin(), blocksSet.end(), std::back_inserter(blocks_));

  // Set header to allow the select column to be clickable
  // to set/unset all selections at once
  QHeaderView *headerView = ui->blocksTable->horizontalHeader();
  headerView->setSectionResizeMode(0, QHeaderView::Stretch);
  headerView->setSectionsClickable(true);

  QObject::connect(headerView, &QHeaderView::sectionClicked, this,
                   &BlocksDialog::tableSectionClicked);

  for (auto block : blocks_) {
    int row = ui->blocksTable->rowCount();
    ui->blocksTable->insertRow(row);

    QTableWidgetItem *item = new QTableWidgetItem;
    item->setData(Qt::DisplayRole, uBlocks[block->blockIdx_].caption_);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    ui->blocksTable->setItem(row, 0, item);

    item = new QTableWidgetItem;
    item->setData(Qt::DisplayRole,
                  QString("U+%1").arg(uBlocks[block->blockIdx_].first_, 5, 16, QChar('0')));
    item->setTextAlignment(Qt::AlignHCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    ui->blocksTable->setItem(row, 1, item);

    item = new QTableWidgetItem;
    item->setTextAlignment(Qt::AlignHCenter);
    item->setData(Qt::DisplayRole,
                  QString("U+%1").arg(int(uBlocks[block->blockIdx_].last_), 5, 16, QChar('0')));
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    ui->blocksTable->setItem(row, 2, item);

    item = new QTableWidgetItem;
    item->setData(Qt::DisplayRole, block->codePointCount_);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setTextAlignment(Qt::AlignHCenter);
    ui->blocksTable->setItem(row, 3, item);

    QWidget     *frame          = new QFrame();
    QCheckBox   *checkBox       = new QCheckBox();
    QHBoxLayout *layoutCheckBox = new QHBoxLayout(frame);
    layoutCheckBox->addWidget(checkBox);
    layoutCheckBox->setAlignment(Qt::AlignCenter);
    layoutCheckBox->setContentsMargins(0, 0, 0, 0);
    checkBox->setObjectName(QString("%1").arg(row));
    ui->blocksTable->setCellWidget(row, 4, frame);

    QObject::connect(checkBox, &QCheckBox::clicked, this, &BlocksDialog::cbClicked);
  }

  allChecked_   = false;
  codePointQty_ = 0;
  updateQtyLabel();
}

BlocksDialog::~BlocksDialog() {
  delete ui;
  FT_Done_FreeType(ftLib_);
}

void BlocksDialog::tableSectionClicked(int idx) {
  if (idx == 4) {
    allChecked_ = !allChecked_;
    for (int i = 0; i < ui->blocksTable->rowCount(); i++) {
      QFrame    *frame = (QFrame *) (ui->blocksTable->cellWidget(i, 4));
      QCheckBox *cb    = (QCheckBox *) (frame->layout()->itemAt(0)->widget());
      cb->setChecked(allChecked_);
    }
    codePointQty_ = allChecked_ ? ftFace_->num_glyphs : 0;
    updateQtyLabel();
  }
}

void BlocksDialog::cbClicked(bool checked) {
  QCheckBox *sender = (QCheckBox *) QObject::sender();
  int        row    = sender->objectName().toInt();
  int        qty    = ui->blocksTable->item(row, 3)->data(Qt::DisplayRole).toInt();
  codePointQty_ += checked ? qty : -qty;
  updateQtyLabel();
}

void BlocksDialog::updateQtyLabel() {
  ui->qtyLabel->setText(QString("Number of CodePoints selected: %1").arg(codePointQty_));
}

void BlocksDialog::on_buttonBox_accepted() {
  selectedBlockIndexes_.clear();
  for (int row = 0; row < ui->blocksTable->rowCount(); row++) {
    QFrame    *frame = (QFrame *) (ui->blocksTable->cellWidget(row, 4));
    QCheckBox *cb    = (QCheckBox *) (frame->layout()->itemAt(0)->widget());
    if (cb->isChecked()) { selectedBlockIndexes_.insert(blocks_[row]->blockIdx_); }
  }
  accept();
}

void BlocksDialog::on_buttonBox_rejected() {
  reject();
}
