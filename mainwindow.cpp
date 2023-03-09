#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <iostream>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
  initialized = false;
  ui->setupUi(this);

  currentFilename = "";
  fontChanged     = false;
  glyphChanged    = false;
  faceChanged     = false;
  glyphReloading  = false;
  faceReloading   = false;

  setWindowTitle("IBMF Font Editor");

  bitmapRenderer = new BitmapRenderer(ui->bitmapFrame, 20, false);
  ui->bitmapFrame->layout()->addWidget(bitmapRenderer);

  QObject::connect(bitmapRenderer, &BitmapRenderer::bitmapHasChanged , this, &MainWindow::bitmapChanged);

  smallGlyph = new BitmapRenderer(ui->smallGlyphPreview, 2, true);
  ui->smallGlyphPreview->layout()->addWidget(smallGlyph);
  smallGlyph->connectTo(bitmapRenderer);

  largeGlyph = new BitmapRenderer(ui->largeGlyphPreview, 4, true);
  ui->largeGlyphPreview->layout()->addWidget(largeGlyph);
  largeGlyph->connectTo(bitmapRenderer);

  centerScrollBarPos();

  ibmfFont = nullptr;
  ibmfGlyphCode = 0;

  // All captions in the tables are not editable
  for (int row = 0; row < ui->fontHeader->rowCount(); row++) {
    this->clearEditable(ui->fontHeader, row, 0);
  }
  QHeaderView* header = ui->fontHeader->horizontalHeader();
  header->setStretchLastSection(true);

  for (int row = 0; row < ui->faceHeader->rowCount(); row++) {
    this->clearEditable(ui->faceHeader, row, 0);
  }
  header = ui->faceHeader->horizontalHeader();
  header->setStretchLastSection(true);

  for (int row = 0; row < ui->characterMetrics->rowCount(); row++) {
    this->clearEditable(ui->characterMetrics, row, 0);
  }
  header = ui->characterMetrics->horizontalHeader();
  header->setStretchLastSection(true);

  header = ui->ligTable->horizontalHeader();
  header->setStretchLastSection(true);

  header = ui->kernTable->horizontalHeader();
  header->setStretchLastSection(true);

  // Initialize the characters table
  ui->charactersList->setColumnCount(5);
  ui->charactersList->setRowCount((174 + 4) / 5);

  int idx = 0;
  for (int row = 0; row < ui->charactersList->rowCount(); row++) {
    ui->charactersList->setRowHeight(row, 50);
    for (int col = 0; col < 5; col++) {
      auto item = new QTableWidgetItem;
      item->setFlags(item->flags() & ~ Qt::ItemIsEditable);
      item->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
      if (idx < 174) {
          item->setData(Qt::EditRole, fontFormat0CharacterCodes[idx]);

          item->setToolTip(QString("%1  0o%2  0x%3")
                              .arg(idx)
                              .arg(idx, 3,  8, QChar('0'))
                              .arg(idx, 2, 16, QChar('0')));
      }
      else {
          item->setFlags(Qt::NoItemFlags);
      }
      ui->charactersList->setItem(row, col, item);
      idx += 1;
    }
  }

  header = ui->charactersList->horizontalHeader();
  header->setSectionResizeMode(QHeaderView::Stretch);

  // Set tables columns widths

  ui->fontHeader      ->setColumnWidth(0, 120);
  ui->faceHeader      ->setColumnWidth(0, 120);
  ui->characterMetrics->setColumnWidth(0, 120);
  ui->fontHeader      ->setColumnWidth(1, 100);
  ui->faceHeader      ->setColumnWidth(1, 100);
  ui->characterMetrics->setColumnWidth(1, 100);

  // clear all data
  this->clearAll();

  auto setHPolicy = [](auto widget1, auto widget2, int value1, int value2) {
    QSizePolicy sp;
    sp = widget1->sizePolicy();
    sp.setHorizontalStretch(value1);
    widget1->setSizePolicy(sp);
    sp = widget2->sizePolicy();
    sp.setHorizontalStretch(value2);
    widget2->setSizePolicy(sp);
  };

  auto setVPolicy = [](auto widget1, auto widget2, int value1, int value2) {
    QSizePolicy sp;
    sp = widget1->sizePolicy();
    sp.setVerticalStretch(value1);
    widget1->setSizePolicy(sp);
    sp = widget2->sizePolicy();
    sp.setVerticalStretch(value2);
    widget2->setSizePolicy(sp);
  };

  ui->leftFrame        ->setSizes(QList<int>() << 50 << 200);
  ui->FaceCharsSplitter->setSizes(QList<int>() << 100 << 200);

  setHPolicy(ui->leftFrame,       ui->rightSplitter,    12, 20);
  setHPolicy(ui->centralFrame,    ui->rightFrame,       20,  8);
  setVPolicy(ui->charsMetrics,    ui->tabs,             20, 20);
  setVPolicy(ui->fontHeaderFrame, ui->FaceCharsSplitter, 3,  4);
  //setVPolicy(ui->faceFrame,       ui->charsFrame,        1,  4);

  initialized = true;

  show();
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  if (!checkFontChanged()) event->ignore();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
}

void MainWindow::paintEvent(QPaintEvent *event)
{
  setScrollBarSizes(bitmapRenderer->getPixelSize());
  updateBitmapOffsetPos();
}

void MainWindow::bitmapChanged(Bitmap & bitmap)
{
  if (ibmfFont != nullptr) {
    fontChanged = true;
    glyphChanged = true;
  }
}

bool MainWindow::saveFont()
{
  QString newFilename;
  QRegularExpression dateTime("(_\\d\\d\\d\\d_\\d\\d_\\d\\d_\\d\\d_\\d\\d_\\d\\d\\.ibmf$)");
  QRegularExpressionMatch match = dateTime.match(currentFilename);
  if (!match.hasMatch()) {
    QRegularExpression extension("(\\.ibmf$)");
    match = extension.match(currentFilename);
  }
  if (match.hasMatch()) {

  }
  return true;
}

bool MainWindow::loadFont(QFile &file)
{
  QByteArray content = file.readAll();
  file.close();
  clearAll();
  ibmfFont = IBMFFontModPtr(new IBMFFontMod((uint8_t *) content.data(), content.size()));
  if (ibmfFont->is_initialized()) {
    ibmfPreamble = ibmfFont->get_preample();

    char marker[5];
    memcpy(marker, ibmfPreamble.marker, 4);
    marker[4] = 0;

    putValue(ui->fontHeader, 0, 1, QByteArray(marker),         false);
    putValue(ui->fontHeader, 1, 1, ibmfPreamble.faceCount,    false);
    putValue(ui->fontHeader, 2, 1, ibmfPreamble.bits.version,  false);
    putValue(ui->fontHeader, 3, 1, ibmfPreamble.bits.fontFormat, false);

    for (int i = 0; i < ibmfPreamble.faceCount; i++) {
      IBMFDefs::FaceHeaderPtr face_header = ibmfFont->get_face_header(i);
      ui->faceIndex->addItem(QString::number(face_header->pointSize).append(" pts"));
    }

    loadFace(0);

    fontChanged = false;
    faceChanged = false;
    glyphChanged = false;
  }
  return ibmfFont->is_initialized();
}


bool MainWindow::checkFontChanged()
{
  if (fontChanged) {
    saveGlyph();
    QMessageBox::StandardButton button = QMessageBox::question(
            this, "File Changed", "File was changed. Do you want to save it ?",
            QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
    if (button == QMessageBox::Yes) {
      QMessageBox::information(this, "Saving....", "Saving the file...");
    }
    else {
      return button != QMessageBox::Cancel;
    }

    fontChanged = false;
    currentFilename = "";
    setWindowTitle("IBMF Font Editor");
  }
  return true;
}

void MainWindow::on_actionOpen_triggered()
{
  if (!checkFontChanged()) return;

  QFile file;

  QString filename = QFileDialog::getOpenFileName(this, "Open IBMF Font File");
  file.setFileName(filename);
  currentFilename = filename;
  fontChanged = false;

  if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(this, "Warning", "Unable to open file " + file.errorString());
  }
  else {
    if (loadFont(file)) {
      setWindowTitle("IBMF Font Editor - " + filename);
    }
    else {
      QMessageBox::warning(this, "Warning", "Unable to load IBMF file " + currentFilename);
    }
    file.close();
  }
}

void MainWindow::clearAll()
{
  ui->faceIndex->clear();
  bitmapRenderer->clearAndEmit(); // Will clear other renderers through signaling
}

void MainWindow::on_actionExit_triggered()
{
  if (checkFontChanged()) QApplication::exit();
}

void MainWindow::clearEditable(QTableWidget * w, int row, int col )
{
  auto item = w->item(row, col);
  item->setFlags(item->flags() & ~ Qt::ItemIsEditable);
}

void MainWindow::putValue(QTableWidget * w, int row, int col, QVariant value, bool editable)
{
  auto item = new QTableWidgetItem();
  item->setData(Qt::EditRole, value);
  if (!editable) item->setFlags(item->flags() & ~ Qt::ItemIsEditable);
  w->setItem(row, col, item);
}

void MainWindow::putFix16Value(QTableWidget * w, int row, int col, QVariant value, bool editable)
{
  auto item = new QTableWidgetItem();
  item->setData(Qt::EditRole, value);
  if (!editable) item->setFlags(item->flags() & ~ Qt::ItemIsEditable);
  w->setItem(row, col, item);
  QAbstractItemDelegate * delegate;
  if ((delegate = w->itemDelegateForRow(row)) != nullptr) delete delegate;
  w->setItemDelegateForRow(row, new Fix16Delegate);
}

QVariant MainWindow::getValue(QTableWidget * w, int row, int col)
{
  return w->item(row, col)->data(Qt::DisplayRole);
}

void MainWindow::saveFace()
{
  if ((ibmfFont != nullptr) && ibmfFont->is_initialized() && faceChanged) {
    IBMFDefs::FaceHeader face_header;

    face_header.pointSize          = getValue(ui->faceHeader,  0, 1).toUInt();
    face_header.lineHeight         = getValue(ui->faceHeader,  1, 1).toUInt();
    face_header.dpi                 = getValue(ui->faceHeader,  2, 1).toUInt();
    face_header.xHeight            = getValue(ui->faceHeader,  3, 1).toFloat() * 64.0;
    face_header.emSize             = getValue(ui->faceHeader,  4, 1).toFloat() * 64.0;
    face_header.slantCorrection    = getValue(ui->faceHeader,  5, 1).toFloat() * 64.0;
    face_header.maxHeight          = getValue(ui->faceHeader,  6, 1).toUInt();
    face_header.descenderHeight    = getValue(ui->faceHeader,  7, 1).toUInt();
    face_header.spaceSize          = getValue(ui->faceHeader,  8, 1).toUInt();
    face_header.glyphCount         = getValue(ui->faceHeader,  9, 1).toUInt();
    face_header.ligKernStepCount = getValue(ui->faceHeader, 10, 1).toUInt();
    face_header.kernCount          = getValue(ui->faceHeader, 11, 1).toUInt();

    ibmfFont->save_face_header(ibmfFaceIdx, face_header);
    faceChanged = false;
  }
}

bool MainWindow::loadFace(uint8_t face_idx)
{
  saveFace();
  if ((ibmfFont != nullptr) && (ibmfFont->is_initialized()) && (face_idx < ibmfPreamble.faceCount)) {
    ibmfFaceHeader = ibmfFont->get_face_header(face_idx);

    faceReloading = true;

    putValue(     ui->faceHeader,  0, 1, ibmfFaceHeader->pointSize,                       false);
    putValue(     ui->faceHeader,  1, 1, ibmfFaceHeader->lineHeight);
    putValue(     ui->faceHeader,  2, 1, ibmfFaceHeader->dpi);
    putFix16Value(ui->faceHeader,  3, 1, (float) ibmfFaceHeader->xHeight / 64.0);
    putFix16Value(ui->faceHeader,  4, 1, (float) ibmfFaceHeader->emSize / 64.0);
    putFix16Value(ui->faceHeader,  5, 1, (float) ibmfFaceHeader->slantCorrection / 64.0);
    putValue(     ui->faceHeader,  6, 1, ibmfFaceHeader->maxHeight);
    putValue(     ui->faceHeader,  7, 1, ibmfFaceHeader->descenderHeight);
    putValue(     ui->faceHeader,  8, 1, ibmfFaceHeader->spaceSize);
    putValue(     ui->faceHeader,  9, 1, ibmfFaceHeader->glyphCount,                      false);
    putValue(     ui->faceHeader, 10, 1, ibmfFaceHeader->ligKernStepCount,              false);
    putValue(     ui->faceHeader, 11, 1, ibmfFaceHeader->kernCount,                       false);

    faceReloading = false;

    ibmfFaceIdx = face_idx;

    loadGlyph(ibmfGlyphCode);
  }
  else {
    return false;
  }

  return true;
}

void MainWindow::saveGlyph()
{
  if ((ibmfFont != nullptr) && ibmfFont->is_initialized() && glyphChanged) {
    Bitmap * theBitmap;
    IBMFDefs::GlyphInfo glyph_info;
    if (bitmapRenderer->retrieveBitmap(&theBitmap)) {

      glyph_info.charCode                   = getValue(ui->characterMetrics,  0, 1).toUInt();
      glyph_info.bitmapWidth                = theBitmap->dim.width;
      glyph_info.bitmapHeight               = theBitmap->dim.height;
      glyph_info.horizontalOffset           = getValue(ui->characterMetrics,  3, 1).toInt();
      glyph_info.verticalOffset             = getValue(ui->characterMetrics,  4, 1).toInt();
      glyph_info.ligKernPgmIndex          = getValue(ui->characterMetrics,  5, 1).toUInt();
      glyph_info.packetLength               = getValue(ui->characterMetrics,  6, 1).toUInt();
      glyph_info.advance                     = getValue(ui->characterMetrics,  7, 1).toFloat() * 64.0;
      glyph_info.rleMetrics.dynF           = getValue(ui->characterMetrics,  8, 1).toUInt();
      glyph_info.rleMetrics.firstIsBlack  = getValue(ui->characterMetrics,  9, 1).toUInt();

      ibmfFont->save_glyph(ibmfFaceIdx, ibmfGlyphCode, &glyph_info, theBitmap);
      glyphChanged = false;
    }
  }
}

bool MainWindow::loadGlyph(uint16_t glyph_code)
{
  saveGlyph();
  if ((ibmfFont != nullptr) &&
      (ibmfFont->is_initialized()) &&
      (ibmfFaceIdx < ibmfPreamble.faceCount) &&
      (glyph_code < ibmfFaceHeader->glyphCount)) {

    if (ibmfFont->get_glyph(ibmfFaceIdx, glyph_code, ibmfGlyphInfo, &ibmfGlyphBitmap)) {
      ibmfGlyphCode = glyph_code;

      glyphChanged = false;
      glyphReloading = true;

      putValue(ui->characterMetrics,  0, 1, ibmfGlyphInfo->charCode,                   false);
      putValue(ui->characterMetrics,  1, 1, ibmfGlyphInfo->bitmapWidth,                false);
      putValue(ui->characterMetrics,  2, 1, ibmfGlyphInfo->bitmapHeight,               false);
      putValue(ui->characterMetrics,  3, 1, ibmfGlyphInfo->horizontalOffset);
      putValue(ui->characterMetrics,  4, 1, ibmfGlyphInfo->verticalOffset);
      putValue(ui->characterMetrics,  5, 1, ibmfGlyphInfo->ligKernPgmIndex,          false);
      putValue(ui->characterMetrics,  6, 1, ibmfGlyphInfo->packetLength,               false);
      putFix16Value(ui->characterMetrics,  7, 1, (float) ibmfGlyphInfo->advance / 64.0);
      putValue(ui->characterMetrics,  8, 1, ibmfGlyphInfo->rleMetrics.dynF,          false);
      putValue(ui->characterMetrics,  9, 1, ibmfGlyphInfo->rleMetrics.firstIsBlack, false);

      centerScrollBarPos();
      bitmapRenderer->clearAndLoadBitmap(*ibmfGlyphBitmap);
      smallGlyph->clearAndLoadBitmap(*ibmfGlyphBitmap);
      largeGlyph->clearAndLoadBitmap(*ibmfGlyphBitmap);

      ui->ligTable->clearContents();
      ui->kernTable->clearContents();

      if (ibmfFont->get_glyph_lig_kern(ibmfFaceIdx, ibmfGlyphCode, &ibmfLigKerns)) {
        ui->ligTable->setRowCount(ibmfLigKerns->lig_steps.size());
        for (int i = 0; i < ibmfLigKerns->lig_steps.size(); i++) {
          putValue(ui->ligTable, i, 0, QChar(fontFormat0CharacterCodes[ibmfLigKerns->lig_steps[i]->nextcharCode]));
          putValue(ui->ligTable, i, 1, QChar(fontFormat0CharacterCodes[ibmfLigKerns->lig_steps[i]->charCode]));
          int code = ibmfLigKerns->lig_steps[i]->nextcharCode;
          ui->ligTable->item(i, 0)->setToolTip(
                QString("%1  0o%2  0x%3")
                  .arg(code)
                  .arg(code, 3,  8, QChar('0'))
                  .arg(code, 2, 16, QChar('0')));
          code = ibmfLigKerns->lig_steps[i]->charCode;
          ui->ligTable->item(i, 1)->setToolTip(
                QString("%1  0o%2  0x%3")
                  .arg(code)
                  .arg(code, 3,  8, QChar('0'))
                  .arg(code, 2, 16, QChar('0')));
        }
        ui->kernTable->setRowCount(ibmfLigKerns->kern_steps.size());
        for (int i = 0; i < ibmfLigKerns->kern_steps.size(); i++) {
          putValue(ui->kernTable, i, 0, QChar(fontFormat0CharacterCodes[ibmfLigKerns->kern_steps[i]->nextcharCode]));
          putFix16Value(ui->kernTable, i, 1, (float) ibmfLigKerns->kern_steps[i]->kern / 64.0);
          int code = ibmfLigKerns->kern_steps[i]->nextcharCode;
          ui->kernTable->item(i, 0)->setToolTip(
                QString("%1  0o%2  0x%3")
                  .arg(code)
                  .arg(code, 3,  8, QChar('0'))
                  .arg(code, 2, 16, QChar('0')));
        }
      }

      ui->charactersList->setCurrentCell(glyph_code / 5, glyph_code % 5);

      glyphReloading = false;
    }
    else {
      return false;
    }
  }
  else {
    return false;
  }

  return true;
}

void MainWindow::on_leftButton_clicked()
{
  if ((ibmfFont == nullptr) || !ibmfFont->is_initialized()) return;

  uint16_t glyph_code = ibmfGlyphCode;

  if (glyph_code > 0) {
    glyph_code -= 1;
  }
  else {
    glyph_code = ibmfFaceHeader->glyphCount - 1;
  }
  loadGlyph(glyph_code);
}


void MainWindow::on_rightButton_clicked()
{
  if ((ibmfFont == nullptr) || !ibmfFont->is_initialized()) return;

  uint16_t glyphCode = ibmfGlyphCode;

  if (glyphCode >= ibmfFaceHeader->glyphCount - 1) {
    glyphCode = 0;
  }
  else {
    glyphCode += 1;
  }
  loadGlyph(glyphCode);
}


void MainWindow::on_faceIndex_currentIndexChanged(int index)
{
  if ((ibmfFont == nullptr) || !ibmfFont->is_initialized()) return;

  loadFace(index);
}

void MainWindow::setScrollBarSizes(int pixelSize)
{
    ui->bitmapHorizontalScrollBar->setPageStep((ui->bitmapFrame->width() / pixelSize) * ((float)ui->bitmapHorizontalScrollBar->maximum() / BitmapRenderer::bitmapWidth));
    ui->bitmapVerticalScrollBar->setPageStep((ui->bitmapFrame->height() / pixelSize) * ((float)ui->bitmapVerticalScrollBar->maximum() / BitmapRenderer::bitmapHeight));
}

void MainWindow::centerScrollBarPos()
{
    ui->bitmapHorizontalScrollBar->setValue(ui->bitmapHorizontalScrollBar->maximum() / 2);
    ui->bitmapVerticalScrollBar->setValue(ui->bitmapVerticalScrollBar->maximum() / 2);
}

void MainWindow::updateBitmapOffsetPos()
{
  QPoint pos = QPoint((float) ui->bitmapHorizontalScrollBar->value()
                          / ui->bitmapHorizontalScrollBar->maximum()
                          * BitmapRenderer::bitmapWidth
                          - ((bitmapRenderer->width() / bitmapRenderer->getPixelSize()) / 2),
                      (float) ui->bitmapVerticalScrollBar->value()
                          / ui->bitmapVerticalScrollBar->maximum()
                          * BitmapRenderer::bitmapHeight
                          - ((bitmapRenderer->height() / bitmapRenderer->getPixelSize()) / 2) );

  int maxX = BitmapRenderer::bitmapWidth - (bitmapRenderer->width() / bitmapRenderer->getPixelSize());
  int maxY = BitmapRenderer::bitmapHeight - (bitmapRenderer->height() / bitmapRenderer->getPixelSize());

  if (pos.x() < 0) pos.setX(0);
  if (pos.y() < 0) pos.setY(0);
  if (pos.x() > maxX) pos.setX(maxX);
  if (pos.y() > maxY) pos.setY(maxY);

#if 0
  std::cout << "Horizontal scrollbar position: "
            << ui->bitmapHorizontalScrollBar->minimum()
            << " < "
            << ui->bitmapHorizontalScrollBar->value()
            << " < "
            << ui->bitmapHorizontalScrollBar->maximum()
            << std::endl;
  std::cout << "Vertical scrollbar position: "
            << ui->bitmapVerticalScrollBar->minimum()
            << " < "
            << ui->bitmapVerticalScrollBar->value()
            << " < "
            << ui->bitmapVerticalScrollBar->maximum()
            << std::endl;

  std::cout << "BitmapRender Size: " << bitmapRenderer->width() << ", " << bitmapRenderer->height() << std::endl;
  std::cout << "BitmapRender Pixel Size: " << bitmapRenderer->getPixelSize() << std::endl;
  std::cout << "BitmapOffsetPos = " << pos.x() << ", " << pos.y() << std::endl;
#endif

  bitmapRenderer->setBitmapOffsetPos(pos);
}

void MainWindow::on_pixelSize_valueChanged(int value)
{
  bitmapRenderer->setPixelSize(ui->pixelSize->value());

  setScrollBarSizes(value);
}


void MainWindow::on_charactersList_cellClicked(int row, int column)
{
  int idx = (row * 5) + column;
  if (ibmfFont != nullptr) {
    loadGlyph(idx);
  }
  else {
    ibmfGlyphCode = idx; // Retain the selection for when a font will be loaded
  }
}

void MainWindow::on_characterMetrics_cellChanged(int row, int column)
{
  if ((ibmfFont != nullptr) && ibmfFont->is_initialized() && initialized && !glyphReloading) {
    glyphChanged = true;
  }
}

void MainWindow::on_faceHeader_cellChanged(int row, int column)
{
  if ((ibmfFont != nullptr) && ibmfFont->is_initialized() && initialized && !faceReloading) {
    faceChanged = true;
  }

}

void MainWindow::on_glyphForgetButton_clicked()
{
  if (glyphChanged) {
    QMessageBox::StandardButton button = QMessageBox::question(
            this, "Character has been Modified",
            "You will loose all changes to the bitmap and metrics. Are-you sure?",
            QMessageBox::Yes|QMessageBox::No);
    if (button == QMessageBox::Yes) {
      glyphChanged = false;
      loadGlyph(ibmfGlyphCode);
    }
  }
}

void MainWindow::on_faceForgetButton_clicked()
{
  if (faceChanged) {
    QMessageBox::StandardButton button = QMessageBox::question(
            this, "Face Header Modified",
            "You will loose all changes to the Face Header. Are-you sure?",
            QMessageBox::Yes|QMessageBox::No);
    if (button == QMessageBox::Yes) {
      faceChanged = false;
      loadFace(ibmfFaceIdx);
    }
  }
}

void MainWindow::on_pushButton_clicked()
{
    centerScrollBarPos();
    updateBitmapOffsetPos();
    bitmapRenderer->repaint();
}


void MainWindow::on_leftSplitter_splitterMoved(int pos, int index)
{
}


void MainWindow::on_bitmapVerticalScrollBar_valueChanged(int value)
{
    if (initialized) {
        updateBitmapOffsetPos();
        bitmapRenderer->repaint();
    }
}

void MainWindow::on_bitmapHorizontalScrollBar_valueChanged(int value)
{
    if (initialized) {
        updateBitmapOffsetPos();
        bitmapRenderer->repaint();
    }
}



void MainWindow::on_actionFont_load_save_triggered()
{
    if (!checkFontChanged()) return;

    QFile in_file;
    QFile out_file;

    QString filename = QFileDialog::getOpenFileName(this, "Open IBMF Font File");
    in_file.setFileName(filename);

    if (!in_file.open(QIODevice::ReadOnly)) {
      QMessageBox::critical(this, "Error", "Unable to open in file " + in_file.errorString());
    }
    else {
      QByteArray original_content = in_file.readAll();
      in_file.close();
      auto font = IBMFFontModPtr(new IBMFFontMod((uint8_t *) original_content.data(), original_content.size()));
      if (font->is_initialized()) {
          filename.append(".test");
          out_file.setFileName(filename);
          if (!out_file.open(QIODevice::WriteOnly)) {
              QMessageBox::critical(this, "Error", "Unable to open out file " + out_file.errorString());
          }
          else {
              QDataStream out(&out_file);
              if (font->save(out)) {
                  out_file.close();
                  QMessageBox::information(this, "Completed", "New font file at " + filename);

              }
              else {
                  out_file.close();
                  QMessageBox::critical(this, "Error", "Unable to save IBMF file " + currentFilename + " error: " + QString::number(font->get_last_error()));
              }
          }
      }
      else {
        QMessageBox::critical(this, "Error", "Unable to load IBMF file " + currentFilename);
      }
    }

}


void MainWindow::on_actionRLE_Encoder_triggered()
{
    if (!checkFontChanged()) return;

    QFile in_file;

    QString filename = QFileDialog::getOpenFileName(this, "Open IBMF Font File");
    in_file.setFileName(filename);

    if (!in_file.open(QIODevice::ReadOnly)) {
      QMessageBox::critical(this, "Error", "Unable to open in file " + in_file.errorString());
    }
    else {
      QByteArray original_content = in_file.readAll();
      in_file.close();
      auto font = IBMFFontModPtr(new IBMFFontMod((uint8_t *) original_content.data(), original_content.size()));
      if (font->is_initialized()) {
        IBMFDefs::GlyphInfoPtr glyph_info;
        Bitmap *bitmapHeight_bits;
//        Bitmap *bitmap_one_bit;
        for (int i = 0; i < 174; i++) {
          if (font->get_glyph(0, i, glyph_info, &bitmapHeight_bits)) {
//            if (!font->convert_to_one_bit(*bitmapHeight_bits, &bitmap_one_bit)) {
//              QMessageBox::critical(this, "Conversion error", "convert_to_one_bit not working properly!");
//            }
            auto gen = new RLEGenerator;
            if (gen->encode_bitmap(*bitmapHeight_bits)) {
              if ((gen->get_data()->size() != glyph_info->packetLength) ||
                  (gen->get_firstIsBlack() != glyph_info->rleMetrics.firstIsBlack) ||
                  (gen->get_dynF() != glyph_info->rleMetrics.dynF)) {
                QMessageBox::critical(this, "Encoder Error",
                                     "Encoder error for char code " +
                                     QString::number(i) +
                                     " Encoder size: " +
                                     QString::number(gen->get_data()->size()) +
                                     " Packet Length: " +
                                     QString::number(glyph_info->packetLength));
              }
            }
            delete gen;
            //delete bitmap_one_bit;
          }
        }
        QMessageBox::information(this, "Completed", "Completed");
      }
      else {
        QMessageBox::critical(this, "Error", "Unable to load IBMF file " + currentFilename);
      }
    }

}

