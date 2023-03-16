#pragma once

#include <QAction>
#include <QByteArray>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QList>
#include <QMainWindow>
#include <QMessageBox>
#include <QString>
#include <QStyledItemDelegate>
#include <QTableWidgetItem>
#include <QUndoStack>
#include <QUndoView>

#include "IBMFDriver/ibmf_font_mod.hpp"
#include "bitmapRenderer.h"

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

protected:
  void closeEvent(QCloseEvent *event);
  void resizeEvent(QResizeEvent *event);
  void paintEvent(QPaintEvent *even);

private slots:
  void on_actionOpen_triggered();
  void on_actionExit_triggered();
  void on_leftButton_clicked();
  void on_rightButton_clicked();
  void on_faceIndex_currentIndexChanged(int index);
  void on_pixelSize_valueChanged(int value);
  void on_charactersList_cellClicked(int row, int column);
  void on_characterMetrics_cellChanged(int row, int column);
  void on_faceHeader_cellChanged(int row, int column);
  void on_glyphForgetButton_clicked();
  void on_faceForgetButton_clicked();
  void bitmapChanged(Bitmap &bitmap);
  void setScrollBarSizes(int value);
  void centerScrollBarPos();
  void updateBitmapOffsetPos();
  void openRecent();
  void on_centerGridButton_clicked();
  void on_leftSplitter_splitterMoved(int pos, int index);
  void on_bitmapVerticalScrollBar_valueChanged(int value);
  void on_bitmapHorizontalScrollBar_valueChanged(int value);
  void on_actionFont_load_save_triggered();
  void on_actionRLE_Encoder_triggered();
  void on_actionSave_triggered();
  void on_actionSaveBackup_triggered();
  void on_clearRecentList_triggered();

private:
  const int MAX_RECENT_FILES = 10;

  Ui::MainWindow *ui;
  QString         currentFilePath;

  QUndoStack *undoStack;
  QUndoView  *undoView;
  QAction    *undoAction;
  QAction    *redoAction;

  bool fontChanged;
  bool faceChanged;
  bool glyphChanged;
  bool initialized;
  bool glyphReloading;
  bool faceReloading;

  BitmapRenderer *bitmapRenderer;
  BitmapRenderer *smallGlyph;
  BitmapRenderer *largeGlyph;

  typedef std::unique_ptr<IBMFFontMod> IBMFFontModPtr;

  IBMFFontModPtr          ibmfFont;
  IBMFDefs::Preamble      ibmfPreamble;
  IBMFDefs::FaceHeaderPtr ibmfFaceHeader;
  IBMFDefs::GlyphInfoPtr  ibmfGlyphInfo;

  IBMFDefs::Bitmap *ibmfGlyphBitmap;

  IBMFFontMod::GlyphLigKern *ibmfLigKerns;

  int              ibmfFaceIdx;
  int              ibmfGlyphCode;
  QList<QAction *> recentFileActionList;

  void     writeSettings();
  void     readSettings();
  void     createUndoView();
  void     createRecentFileActionsAndConnections();
  void     updateRecentActionList();
  void     adjustRecentsForCurrentFile();
  bool     checkFontChanged();
  bool     saveFont(bool askToConfirmName);
  bool     loadFont(QFile &file);
  bool     loadFace(uint8_t face_idx);
  void     saveFace();
  void     saveGlyph();
  bool     loadGlyph(uint16_t glyph_code);
  void     clearAll();
  void     putValue(QTableWidget *w, int row, int col, QVariant value, bool editable = true);
  void     putFix16Value(QTableWidget *w, int row, int col, QVariant value, bool editable = true);
  QVariant getValue(QTableWidget *w, int row, int col);
  void     clearEditable(QTableWidget *w, int row, int col);
};

// The following is to support Fix16 fields editing in QTableWidgets
class Fix16Delegate : public QStyledItemDelegate {
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &style,
                        const QModelIndex &index) const {
    QDoubleSpinBox *box = new QDoubleSpinBox(parent);

    box->setDecimals(4);

    // you can also set these
    box->setSingleStep(0.01);
    box->setMinimum(0);
    box->setMaximum(1000);

    return box;
  }
};
