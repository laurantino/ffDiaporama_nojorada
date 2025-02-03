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

#include "cCShapeComboBox.h"
#include <QPainter>
#include <QKeyEvent>
#include "engine/_Diaporama.h"

//******************************************************************************************************************
// Custom QAbstractItemDelegate for AutoFraming ComboBox
//******************************************************************************************************************
#define AutoFrameShapeComboBoxIMAGEWIDTH   48
#define AutoFrameShapeComboBoxNBRCOLUMN    8

//========================================================================================================================
cFrameShapeTableItem::cFrameShapeTableItem(QImage *Image,int FrameStyle) 
{
    this->Image      = *Image;
    this->FrameStyle = FrameStyle;
}

//========================================================================================================================
void tileModel::PrepareFrameShapeTable(bool /*ResetContent*/,int Filter,int CurrentBackgroundForm)
{
   if ((CurrentBackgroundForm >= SHAPEFORM_TRIANGLEUP) && (CurrentBackgroundForm <= SHAPEFORM_TRIANGLELEFT)) 
      Filter = Filter | FILTERFRAMESHAPE_OLDTRIANGLE;
   FrameShapeTable.clear();
   for (int i = 0; i < NBR_SHAPEFORM; i++) 
   {
      bool ToAdd = ShapeFormDefinition.at(i).Enable;
      if ((i >= SHAPEFORM_TRIANGLEUP) && (i <= SHAPEFORM_TRIANGLELEFT)) 
         ToAdd = ((Filter & FILTERFRAMESHAPE_OLDTRIANGLE) > 0);
      if (ToAdd) 
      {
         QImage   Image(QSize(AutoFrameShapeComboBoxIMAGEWIDTH,AutoFrameShapeComboBoxIMAGEWIDTH),QImage::Format_ARGB32);
         QPainter Painter;
         QPen pen(Qt::black, 2);
         Painter.begin(&Image);
         Painter.setPen(pen);
         Painter.fillRect(QRect(0,0,AutoFrameShapeComboBoxIMAGEWIDTH,AutoFrameShapeComboBoxIMAGEWIDTH),"#ffffff");
         QList<QPolygonF>  polys = ComputePolygon(i,5,5,AutoFrameShapeComboBoxIMAGEWIDTH-10,AutoFrameShapeComboBoxIMAGEWIDTH-10);
         for(int j = 0; j < polys.count(); j++ )
            Painter.drawPolygon(polys.at(j));
         Painter.end();
         FrameShapeTable.append(cFrameShapeTableItem(&Image,i));
      }
   }
}

int tileModel::columnCount(const QModelIndex & /*parent*/) const
{
   return AutoFrameShapeComboBoxNBRCOLUMN;
}

QVariant tileModel::data(const QModelIndex & index, int role ) const
{
   if( role == Qt::SizeHintRole )
      return QSize(AutoFrameShapeComboBoxIMAGEWIDTH,AutoFrameShapeComboBoxIMAGEWIDTH);

   if (!index.isValid())
      return QVariant();

   cFrameShapeTableItem* item = static_cast<cFrameShapeTableItem*>(index.internalPointer());

   int straightIndex = index.row() * AutoFrameShapeComboBoxNBRCOLUMN + index.column();
   /*const*/ cFrameShapeTableItem* p = NULL;
   if( straightIndex >= 0 && straightIndex < FrameShapeTable.size() )
   {
      p = (cFrameShapeTableItem*)&FrameShapeTable.at(straightIndex);
   }
   if( item != p && p != NULL )
      item = p;
   if( role == Qt::DisplayRole )
      return ShapeFormDefinition.at(item->FrameStyle).Name;

   if (role == Qt::DecorationRole)
      return QPixmap::fromImage(item->Image); // QPixmap

   if (role == Qt::ToolTipRole || role == Qt::StatusTipRole)
      return ShapeFormDefinition.at(item->FrameStyle).Name; // QString
    if( role == Qt::UserRole )
      return item->FrameStyle;
   //qDebug() << "no data for role " << Qt::ItemDataRole(role);
   return QVariant();
}

QModelIndex tileModel::index(int row, int column, const QModelIndex & /*parent*/ ) const
{
   int straightIndex = row * AutoFrameShapeComboBoxNBRCOLUMN + column;
   const cFrameShapeTableItem* p = NULL;
   if( straightIndex >= 0 && straightIndex < FrameShapeTable.size() )
   {
      p = &FrameShapeTable.at(straightIndex);
      return createIndex(row, column, (void *)p);
   }
   return QModelIndex();
}

QModelIndex tileModel::parent(const QModelIndex & /*index*/) const
{
   return QModelIndex(); 
}

int tileModel::rowCount(const QModelIndex & /*parent*/) const
{
   int NbrItem = FrameShapeTable.count();
   int NbrRow = NbrItem / AutoFrameShapeComboBoxNBRCOLUMN;
   if (NbrRow * AutoFrameShapeComboBoxNBRCOLUMN < NbrItem) 
      NbrRow++;
   return NbrRow;
}

int tileModel::shapeFromIndex(QModelIndex idx) const
{
   if( !idx.isValid() )
      return -1;
   return idx.row() * AutoFrameShapeComboBoxNBRCOLUMN + idx.column();
}

QModelIndex tileModel::getIndexForShape(int shape) const
{
   int Index = 0;
   while ((Index < FrameShapeTable.count()) && (shape != FrameShapeTable.at(Index).FrameStyle)) 
      Index++;
   if (Index < FrameShapeTable.count())
      return index(Index / AutoFrameShapeComboBoxNBRCOLUMN, Index % AutoFrameShapeComboBoxNBRCOLUMN); 
   return QModelIndex();
}

QModelIndex tileModel::getIndexForIndex(int Index) const
{
	if (Index >= 0 && Index < FrameShapeTable.count())
		return index(Index / AutoFrameShapeComboBoxNBRCOLUMN, Index % AutoFrameShapeComboBoxNBRCOLUMN);
	return QModelIndex();
}

QModelIndex tileModel::next(QModelIndex current)
{
   int idx = current.row() * AutoFrameShapeComboBoxNBRCOLUMN + current.column() + 1;
   if (idx < FrameShapeTable.count())
      return index(idx / AutoFrameShapeComboBoxNBRCOLUMN, idx % AutoFrameShapeComboBoxNBRCOLUMN); 
   return current;
}

QModelIndex tileModel::prev(QModelIndex current)
{
   int idx = current.row() * AutoFrameShapeComboBoxNBRCOLUMN + current.column() - 1;
   if( idx >= 0 )
      return index(idx / AutoFrameShapeComboBoxNBRCOLUMN, idx % AutoFrameShapeComboBoxNBRCOLUMN); 
   return current;
}

QModelIndex tileModel::last()
{
   int idx = FrameShapeTable.count() - 1;
   return index(idx / AutoFrameShapeComboBoxNBRCOLUMN, idx % AutoFrameShapeComboBoxNBRCOLUMN); 
}

//========================================================================================================================
cCShapeComboBoxItem::cCShapeComboBoxItem(QObject *parent) : QStyledItemDelegate(parent) 
{
}

void cCShapeComboBoxItem::paint(QPainter *painter,const QStyleOptionViewItem &option,const QModelIndex &index) const 
{
   int CurIndex = index.row() * AutoFrameShapeComboBoxNBRCOLUMN + index.column();
   int CurrentFramingStyle = 0;
   while ( (CurrentFramingStyle < ComboBox->getFrameShapeTable()->count()) && (ComboBox->GetCurrentFrameShape() != ComboBox->getFrameShapeTable()->at(CurrentFramingStyle).FrameStyle)) 
      CurrentFramingStyle++;

   if ((CurIndex >= 0) && (CurIndex < ComboBox->getFrameShapeTable()->count()) ) 
   {
      painter->drawPixmap(option.rect.left(),option.rect.top(),QPixmap().fromImage(((cFrameShapeTableItem)ComboBox->getFrameShapeTable()->at(CurIndex)).Image));
   } 
   else 
   {
      painter->fillRect(option.rect,Qt::white);
   }
   if (index.row() * ((QTableWidget *)ComboBox->view())->columnCount() + index.column() == CurrentFramingStyle) 
   {
      painter->setPen(QPen(Qt::red));
      painter->setBrush(QBrush(Qt::NoBrush));
      painter->drawRect(option.rect.x()+3,option.rect.y()+3,option.rect.width()-6-1,option.rect.height()-6-1);
   }
   if (option.state & QStyle::State_Selected)  
   {
      painter->setPen(QPen(Qt::blue));
      painter->setBrush(QBrush(Qt::NoBrush));
      painter->drawRect(option.rect.x(),option.rect.y(),option.rect.width()-1,option.rect.height()-1);
      painter->drawRect(option.rect.x()+1,option.rect.y()+1,option.rect.width()-1-2,option.rect.height()-1-2);
      painter->setPen(QPen(Qt::black));
      painter->drawRect(option.rect.x()+2,option.rect.y()+2,option.rect.width()-1-4,option.rect.height()-1-4);
   }
}

QSize cCShapeComboBoxItem::sizeHint(const QStyleOptionViewItem &/*option*/,const QModelIndex &/*index*/) const 
{
    return QSize(AutoFrameShapeComboBoxIMAGEWIDTH,AutoFrameShapeComboBoxIMAGEWIDTH);
}


//========================================================================================================================
cCShapeComboBox::cCShapeComboBox(QWidget *parent) : QComboBox(parent) 
{
    CurrentFilter       = -1;
    CurrentFramingStyle = -1;
    CurrentNbrITem      = -1;
   dontEmit            = false;
    //STOPMAJ             = false;

   QTableView* Table = new QTableView(this);
   Table->horizontalHeader()->hide();
   Table->verticalHeader()->hide();

   model =  new tileModel(this);
   setModel(model);
   setView(Table);

   for (int i = 0; i < AutoFrameShapeComboBoxNBRCOLUMN; i++) 
   {
      Table->setColumnWidth(i,AutoFrameShapeComboBoxIMAGEWIDTH);
   }
   Table->resizeRowsToContents();

   ItemDelegate.ComboBox = this;
   setItemDelegate(&ItemDelegate);
   this->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
   this->view()->setFixedWidth(AutoFrameShapeComboBoxIMAGEWIDTH * AutoFrameShapeComboBoxNBRCOLUMN + 18);

   //connect(Table,SIGNAL(itemSelectionChanged()),this,SLOT(s_ItemSelectionChanged()));
   //connect(Table->selectionModel(),SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),this,SLOT(s_ItemSelectionChanged(const QItemSelection &, const QItemSelection &)));
   //connect(Table->selectionModel(),SIGNAL(currentChanged(const QModelIndex &, const QModelIndex&)),this,SLOT(s_ItemSelectionCurrentChanged(const QModelIndex &, const QModelIndex &)));

   connect(this,SIGNAL(activated(int)),this,SLOT(slotActivated(int)));
   connect(this,SIGNAL(currentIndexChanged(int)),this,SLOT(currentIndexChanged(int)));
}

void cCShapeComboBox::SetCurrentFrameShape(int FrameShape)
{
   //qDebug() << "cCShapeComboBox::SetCurrentFrameShape(int FrameShape) "  << FrameShape;
   //if( FrameShape == CurrentFramingStyle )
   //   return;
   CurrentFramingStyle = FrameShape;

   //int Index = 0;
   QModelIndex index = model->getIndexForShape(CurrentFramingStyle);
   if( index.isValid() )
   {
      dontEmit = true;
      ((QTableView *)view())->setCurrentIndex(index);
      setModelColumn(index.column());
      dontEmit = false;
	  QComboBox::setCurrentIndex(index.row());
   }
}

int cCShapeComboBox::GetCurrentFrameShape()
{
   return CurrentFramingStyle;
}

void cCShapeComboBox::keyPressEvent(QKeyEvent *event)
{
   QModelIndex mIndex = ((QTableView *)view())->currentIndex();
   QModelIndex nextIndex;
   if ((event->key() == Qt::Key_Right) || (event->key() == Qt::Key_Down)) 
   {
      nextIndex = model->next(mIndex);
   } 
   else if ((event->key() == Qt::Key_Left) || (event->key() == Qt::Key_Up)) 
   {
      nextIndex = model->prev(mIndex);
   } 
   else if(event->key() == Qt::Key_Home) 
      nextIndex = model->home();
   else if(event->key() == Qt::Key_End) 
      nextIndex = model->last();
   else 
   {
      QComboBox::keyPressEvent(event);
      return;
   }

   if (mIndex != nextIndex) 
   {
      ((QTableView *)view())->setCurrentIndex(nextIndex);
      setModelColumn(nextIndex.column());
	  QComboBox::setCurrentIndex(nextIndex.row());
      CurrentFramingStyle = ((cFrameShapeTableItem*)(nextIndex.internalPointer()))->FrameStyle;
      emit itemSelectionHaveChanged();
      //emit itemSelectionHaveChanged(GetCurrentFrameShape());
   }
}

void cCShapeComboBox::wheelEvent(QWheelEvent *e)
{
   if( view()->isVisible() ) 
      return;
   QModelIndex mIndex = ((QTableView *)view())->currentIndex();
   QModelIndex nextIndex;
   int delta = 0;
   if (!e->pixelDelta().isNull())
      delta = e->pixelDelta().y();
   else if (!e->angleDelta().isNull())
      delta = e->angleDelta().y();
   if (delta > 0) 
   {
      nextIndex = model->prev(mIndex);
   } 
   else if (delta < 0) 
   {
      nextIndex = model->next(mIndex);
   }
   if ( nextIndex != mIndex ) 
   {
      ((QTableView *)view())->setCurrentIndex(nextIndex);
      setModelColumn(nextIndex.column());
	  QComboBox::setCurrentIndex(nextIndex.row());
   }
   e->accept();
   //qDebug() << "wheelEvent done";
}

void cCShapeComboBox::currentIndexChanged(int index)
{
   Q_UNUSED(index)
   //qDebug() << "cCShapeComboBox::currentIndexChanged(int index) " << index;
   QModelIndex idx = view()->currentIndex();
   if( idx.isValid() )
   {
      cFrameShapeTableItem* item = static_cast<cFrameShapeTableItem*>(idx.internalPointer());
      if( !dontEmit )
      {
         CurrentFramingStyle = item->FrameStyle;
         emit itemSelectionHaveChanged();
      }
   }
}

void cCShapeComboBox::setCurrentIndex(int index)
{
	QModelIndex mindex = model->getIndexForIndex(index);
	if (mindex.isValid())
	{
		dontEmit = true;
		((QTableView *)view())->setCurrentIndex(mindex);
		setModelColumn(mindex.column());
		dontEmit = false;
		QComboBox::setCurrentIndex(mindex.row());
	}
}

void cCShapeComboBox::slotActivated(int index)
{
   Q_UNUSED(index)
   //qDebug() << "cCShapeComboBox::slotActivated(int index) " << index;
   QModelIndex idx = view()->currentIndex();
   if (!idx.isValid())
      return;
   setModelColumn(idx.column());
}

void cCShapeComboBox::s_ItemSelectionChanged(const QItemSelection &selected, const QItemSelection &)
{
   
   QModelIndexList indizes = selected.indexes();
   //if( indizes.size() > 0 )
   //{
   //   qDebug() << "new selected item: " << indizes.at(0).row() << " " << indizes.at(0).column();   
   //}
}

void cCShapeComboBox::s_ItemSelectionCurrentChanged(const QModelIndex& /*current*/, const QModelIndex &)
{
      //qDebug() << "current item: " << current.row() << " " << current.column();   
}

