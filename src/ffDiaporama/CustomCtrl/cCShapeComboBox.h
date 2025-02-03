/* ======================================================================
    This file is part of ffDiaporama
    ffDiaporama is a tool to make diaporama as video
    Copyright (C) 2011-2014 Dominique Levray <domledom@laposte.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
   ====================================================================== */

#ifndef CCSHAPECOMBOBOX_H
#define CCSHAPECOMBOBOX_H

// Basic inclusions (common to all files)
#include "_QCustomDialog.h"
#include "Shape.h"

//******************************************************************************************************************
// Custom Frame shape ComboBox
//******************************************************************************************************************

#define FILTERFRAMESHAPE_OLDTRIANGLE        0x01

class cFrameShapeTableItem 
{
public:
    QImage Image;
    int    FrameStyle;
    cFrameShapeTableItem(QImage *Image,int FrameStyle);
};

class tileModel : public QAbstractItemModel
{
   QList<cFrameShapeTableItem> FrameShapeTable;
   public:
      tileModel(QObject *parent = 0 ) : QAbstractItemModel(parent) { PrepareFrameShapeTable(true,0,0); }
      void PrepareFrameShapeTable(bool ResetContent,int Filter,int CurrentBackgroundForm);
      virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;
      virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
      virtual QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
      virtual QModelIndex parent(const QModelIndex & index) const;
      virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;

      int shapeFromIndex(QModelIndex idx) const;
      QModelIndex getIndexForShape(int shape) const;
	  QModelIndex getIndexForIndex(int index) const;
      QModelIndex home() { return index(0,0); }
      QModelIndex next(QModelIndex current);
      QModelIndex prev(QModelIndex current);
      QModelIndex last();

      const QList<cFrameShapeTableItem> *getFrameShapeTable() const { return &FrameShapeTable; }
};

class cCShapeComboBox;
class cCShapeComboBoxItem : public QStyledItemDelegate 
{
   Q_OBJECT
   public:
      cCShapeComboBox *ComboBox;

      explicit cCShapeComboBoxItem(QObject *parent=0);

      virtual void    paint(QPainter *painter,const QStyleOptionViewItem &option,const QModelIndex &index) const;
      virtual QSize   sizeHint(const QStyleOptionViewItem &option,const QModelIndex &index) const;
};

class cCShapeComboBox : public QComboBox 
{
   Q_OBJECT
      cCShapeComboBoxItem ItemDelegate;
      tileModel          *model;
      int                CurrentFilter; 
      int                CurrentFramingStyle;
      int                CurrentNbrITem;
      bool dontEmit;
   public:
      explicit cCShapeComboBox(QWidget *parent = 0);
      void     SetCurrentFrameShape(int FrameShape);
      int      GetCurrentFrameShape();
      const QList<cFrameShapeTableItem> *getFrameShapeTable() const { return model->getFrameShapeTable(); }

   protected:
      virtual void keyPressEvent(QKeyEvent *event);
      virtual void wheelEvent(QWheelEvent *e);

   signals:
      void itemSelectionHaveChanged();
      //void itemSelectionHaveChanged(int FrameShape);

   public slots:
      void s_ItemSelectionChanged(const QItemSelection &, const QItemSelection &);
      void s_ItemSelectionCurrentChanged(const QModelIndex&, const QModelIndex &);
      void slotActivated(int index);
      void currentIndexChanged(int);
	  void setCurrentIndex(int index);
};

#endif // CCSHAPECOMBOBOX_H
