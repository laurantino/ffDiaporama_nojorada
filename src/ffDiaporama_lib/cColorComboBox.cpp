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

#include "cColorComboBox.h"
#include <QColorDialog>
#include <QPainter>
#include <QTableWidget>
#include <QHeaderView>
#include <QKeyEvent>

//******************************************************************************************************************
// Custom Color ComboBox ITEM
//******************************************************************************************************************

#define MAXCOLORREF  99
QString  ColorRef[MAXCOLORREF]={
    "#f2f2f2","#d8d8d8","#bfbfbf","#a5a5a5","#7f7f7f",      // LIGHT-GRAY
    "#595959","#3f3f3f","#262626","#0c0c0c","#000000",      // DARK-GRAY
    "#dae1eb","#b5c4d7","#91a7c3","#3c526f","#28374a",      // BLUE-GRAY
    "#c8eefc","#91defa","#5acef8","#0578a2","#03506c",      // BLUE-1
    "#e0e6f5","#c1ceeb","#a2b5e2","#365bb0","#243c75",      // BLUE-2
    "#e8eeee","#d1dede","#b9cdce","#61888a","#405b5c",      // BLUE-3
    "#e5ecd8","#cbd9b2","#b2c78c","#5c7237","#3d4c25",      // GREEN-1
    "#e8efe8","#d2dfd1","#bbcfba","#648c60","#425d40",      // GREEN-2
    "#e1dca5","#d0c974","#a29a36","#514d1b","#201e0a",      // GREEN-3
    "#f5f2d8","#ece5b2","#e2d88c","#a39428","#6d621a",      // GREEN-4
    "#f2eee8","#e6ded1","#dacdba","#a38557","#6d593a",      // MARROON-1
    "#f6e6d5","#eeceaa","#e6b681","#a2641f","#6c4315",      // MARROON-2
    "#f2e0c6","#e6c28d","#daa454","#664515","#442e0e",      // MARROON-3
    "#fff7c1","#fff084","#ffe947","#998700","#665a00",      // YELLOW-1
    "#fde1d1","#fcc3a3","#fba576","#c94b05","#863203",      // ORANGE
    "#fbc7bc","#f78f7a","#f35838","#711806","#4b1004",      // RED-1
    "#e5e1f4","#cbc3e9","#b1a6de","#533da9","#372970",      // VIOLET-1
    "#ece4f1","#dac9e3","#c7aed6","#7d4d99","#533366",      // VIOLET-2

    "#000000","#ff0000","#00ff00","#0000ff","#ffffff",      // Full-colors
    "#ffff00","#ff00ff","#00ffff","#3a3a3a"
};

//========================================================================================================================

cCColorComboBoxItem::cCColorComboBoxItem(QObject *parent):QStyledItemDelegate(parent) {
}

//========================================================================================================================

void cCColorComboBoxItem::paint(QPainter *painter,const QStyleOptionViewItem &option,const QModelIndex &index) const {
    int ColorNum=index.row()*5+index.column();
    if (ColorNum<MAXCOLORREF) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(QColor(ColorRef[ColorNum])));
        painter->drawRect(option.rect);
        //qDebug() << option.rect;
    } else {
        if (ColorNum==MAXCOLORREF) {
            painter->fillRect(option.rect,QColor(ComboBox->SavedCustomColor));
            QImage  Img(":/img/colorize.png");
            painter->drawImage(QRectF(option.rect.x()+(option.rect.width()-16)/2,option.rect.y()+(option.rect.height()-16)/2,16,16),Img);
        } else {
            painter->fillRect(option.rect,Qt::white);
        }
    }
    if (option.state & QStyle::State_Selected) {
        painter->setPen(QPen(Qt::blue));
        painter->setBrush(QBrush(Qt::NoBrush));
        painter->drawRect(option.rect.x(),option.rect.y(),option.rect.width()-1,option.rect.height()-1);
        painter->drawRect(option.rect.x()+1,option.rect.y()+1,option.rect.width()-1-2,option.rect.height()-1-2);
        painter->setPen(QPen(Qt::black));
        painter->drawRect(option.rect.x()+2,option.rect.y()+2,option.rect.width()-1-4,option.rect.height()-1-4);
    }
}

//========================================================================================================================

QSize cCColorComboBoxItem::sizeHint(const QStyleOptionViewItem &/*option*/,const QModelIndex &/*index*/) const {
    return QSize(32,24);
}

//******************************************************************************************************************
// Custom Color ComboBox
//******************************************************************************************************************

cCColorComboBox::cCColorComboBox(QWidget *parent):QComboBox(parent) {
    STOPMAJ     =false;
    IsPopupOpen =false;
    CurrentColor=NULL;

    QTableWidget    *Table=new QTableWidget();
    Table->horizontalHeader()->hide();
    Table->verticalHeader()->hide();
    Table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    Table->insertColumn(Table->columnCount());  Table->setColumnWidth(Table->columnCount()-1,24);
    Table->insertColumn(Table->columnCount());  Table->setColumnWidth(Table->columnCount()-1,24);
    Table->insertColumn(Table->columnCount());  Table->setColumnWidth(Table->columnCount()-1,24);
    Table->insertColumn(Table->columnCount());  Table->setColumnWidth(Table->columnCount()-1,24);
    Table->insertColumn(Table->columnCount());  Table->setColumnWidth(Table->columnCount()-1,24);
    setModel(Table->model());
    setView(Table);
    int i=0;
    while (i<=MAXCOLORREF) {
        if ((i/5)>=Table->rowCount()) {
            addItem("");        // automaticaly do a Table->insertRow(Table->rowCount());
            Table->setRowHeight(Table->rowCount()-1,24);
        }
        i++;
    }
    ItemDelegate.ComboBox=this;
    setItemDelegate(&ItemDelegate);
    MakeIcons();
    this->view()->setFixedWidth(5*24+18);
    this->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    connect(Table,SIGNAL(itemSelectionChanged()),this,SLOT(s_ItemSelectionChanged()));
    connect(Table,SIGNAL(cellPressed(int,int)),this,SLOT(s_ItemPressed(int,int)));
}

//========================================================================================================================

void cCColorComboBox::SetCurrentColor(QString *Color) {
    CurrentColor=Color;
    if (!Color) return;
    int i=0;
    while ((i<MAXCOLORREF)&&(ColorRef[i]!=*CurrentColor)) i++;
    int Row=i/5;
    int Col=i-(i/5)*5;
    StandardColor=((i>=0)&&(i<MAXCOLORREF));
    if (STOPMAJ) return;
    if (!StandardColor) {
        SavedCustomColor=*CurrentColor;
        ((QTableWidget *)view())->setCurrentCell(Row,5);
        setCurrentIndex(Row);
    } else {
        //SavedCustomColor=ColorRef[i]/*"#fefefe"*/;
        ((QTableWidget *)view())->setCurrentCell(Row,Col);
        setCurrentIndex(Row);
    }
    MakeIcons();
}

//========================================================================================================================

QString cCColorComboBox::GetCurrentColor() {
    if (!CurrentColor) return SavedCustomColor;
    int i=((QTableWidget *)view())->currentRow()*5+((QTableWidget *)view())->currentColumn();
    StandardColor=((i>=0)&&(i<MAXCOLORREF));
    if ((i>=0)&&(i<MAXCOLORREF)) {
        *CurrentColor=ColorRef[i];
        return *CurrentColor;
    }
    return SavedCustomColor;
}

//========================================================================================================================

void cCColorComboBox::MakeIcons() {
    int ColorNum=0;
    if (CurrentColor) {
        while ((ColorNum<MAXCOLORREF)&&(ColorRef[ColorNum]!=*CurrentColor)) ColorNum++;
    } else {
        int CurrentRow=((QTableWidget *)view())->currentRow();      if (CurrentRow<0) CurrentRow=0;
        int CurrentCol=((QTableWidget *)view())->currentColumn();   if (CurrentCol<0) CurrentCol=0;
        ColorNum=CurrentRow*5+CurrentCol;
    }
    //setIconSize(QSize(width()-16,16));
    setIconSize(QSize(24,12));
    QPixmap  Image(iconSize());
    QPainter Painter;
    Painter.begin(&Image);
    if (ColorNum<MAXCOLORREF) {
        Painter.fillRect(QRectF(0,0,iconSize().width(),iconSize().height()),QColor(ColorRef[ColorNum]));
    } else {
        Painter.fillRect(QRectF(0,0,iconSize().width(),iconSize().height()),QColor(SavedCustomColor));
        QImage  Img(":/img/colorize.png");
        //Painter.drawImage(QRectF((iconSize().width()-16)/2,(iconSize().height()-16)/2,16,16),Img,QRectF(0,0,Img.width(),Img.height()));
        Painter.drawImage(QRectF(0,0,8,8),Img,QRectF(0,0,Img.width(),Img.height()));
    }
    Painter.end();
    setItemIcon(currentIndex(),QIcon(Image));
}

//========================================================================================================================

void cCColorComboBox::s_ItemSelectionChanged() {
    STOPMAJ=true;
    setCurrentIndex(((QTableWidget *)view())->currentRow());
    MakeIcons();
    emit currentIndexChanged(((QTableWidget *)view())->currentRow()*5+((QTableWidget *)view())->currentColumn());
    STOPMAJ=false;
}

//========================================================================================================================

void cCColorComboBox::s_ItemPressed(int,int) {
    int CurrentRow=((QTableWidget *)view())->currentRow();      if (CurrentRow<0) CurrentRow=0;
    int CurrentCol=((QTableWidget *)view())->currentColumn();   if (CurrentCol<0) CurrentCol=0;
    int ColorNum=CurrentRow*5+CurrentCol;
    if (ColorNum>=MAXCOLORREF) {
        // Open box to select custom color
        QColor color=QColorDialog::getColor(SavedCustomColor, 0, QString(), QColorDialog::ShowAlphaChannel);
        if (color.isValid()) {
            STOPMAJ=true;
            SavedCustomColor=color.name();
            MakeIcons();
            emit currentIndexChanged(((QTableWidget *)view())->currentRow()*5+((QTableWidget *)view())->currentColumn());
            STOPMAJ=false;
        }
    }
}




// new:
cColorComboBoxItem::cColorComboBoxItem(QObject *parent):QStyledItemDelegate(parent) 
{
}

void cColorComboBoxItem::paint(QPainter *painter,const QStyleOptionViewItem &option,const QModelIndex &index) const 
{
   int ColorNum = index.row() * 5 + index.column();
   if (ColorNum < MAXCOLORREF)
   {
      painter->setPen(Qt::NoPen);
      painter->setBrush(QBrush(QColor(ColorRef[ColorNum])));
      painter->drawRect(option.rect);
      //qDebug() << option.rect;
   }
   else
   {
      if (ColorNum == MAXCOLORREF)
      {
         painter->fillRect(option.rect, QColor(ComboBox->SavedCustomColor));
         QImage  Img(":/img/colorize.png");
         painter->drawImage(QRectF(option.rect.x() + (option.rect.width() - 16) / 2, option.rect.y() + (option.rect.height() - 16) / 2, 16, 16), Img);
      }
      else
      {
         painter->fillRect(option.rect, Qt::white);
      }
   }
   if (option.state & QStyle::State_Selected)
   {
      painter->setPen(QPen(Qt::blue));
      painter->setBrush(QBrush(Qt::NoBrush));
      painter->drawRect(option.rect.x(), option.rect.y(), option.rect.width() - 1, option.rect.height() - 1);
      painter->drawRect(option.rect.x() + 1, option.rect.y() + 1, option.rect.width() - 1 - 2, option.rect.height() - 1 - 2);
      painter->setPen(QPen(Qt::black));
      painter->drawRect(option.rect.x() + 2, option.rect.y() + 2, option.rect.width() - 1 - 4, option.rect.height() - 1 - 4);
   }
}

QSize cColorComboBoxItem::sizeHint(const QStyleOptionViewItem &/*option*/,const QModelIndex &/*index*/) const 
{
    return QSize(24,24);
}

QPixmap pixmapFromColor(QString color)
{
   QImage img(32, 16, QImage::Format_RGB32);
   //QImage img(24, 24, QImage::Format_RGB32);
   QPainter painter(&img);
   painter.setPen(Qt::NoPen);
   painter.setBrush(QBrush(QColor(color)));
   painter.drawRect(QRect(0,0,32,16));
   //painter.drawRect(QRect(0, 0, 24, 24));
   painter.end();
   return QPixmap::fromImage(img);
}

colorModel::colorModel(cColorComboBox *parent ) : QAbstractItemModel(parent) 
{ 
   comboBox = parent; 
}

int colorModel::columnCount(const QModelIndex & /*parent*/) const 
{
   return 5;
}

QVariant colorModel::data(const QModelIndex & index, int role) const
{
   if( role == Qt::SizeHintRole )
      return QSize(24,24);

   if (!index.isValid())
      return QVariant();

   //if( role == Qt::DisplayRole )
   //   return colorFromIndex(index);

   if (role == Qt::DecorationRole)
      return pixmapFromColor(colorFromIndex(index)); // QPixmap

   if (role == Qt::ToolTipRole || role == Qt::StatusTipRole)
      return colorFromIndex(index); // QString

   //qDebug() << "no data for role " << Qt::ItemDataRole(role);
   return QVariant();
}

QModelIndex colorModel::index(int row, int column, const QModelIndex & /*parent*/) const
{
   return createIndex(row,column);
}

QModelIndex colorModel::parent(const QModelIndex & /*index*/) const
{
   return QModelIndex();
}

int colorModel::rowCount(const QModelIndex & /*parent*/ ) const
{
   return (MAXCOLORREF+1)/5;
}

QString colorModel::colorFromIndex(QModelIndex idx) const
{
   int index = idx.row()*5 + idx.column();
   if( index >= MAXCOLORREF )
      return comboBox->SavedCustomColor;
   return ColorRef[index];
}

QModelIndex colorModel::getIndexForColor(QString color) const
{
   int i = 0;
   while ((i < MAXCOLORREF) && (ColorRef[i] != color)) i++;
   int Row = i / 5;
   int Col = i % 5;
   bool StandardColor = ((i >= 0) && (i < MAXCOLORREF));
   if (!StandardColor) 
   {
      return index(MAXCOLORREF/5, MAXCOLORREF%5);
   }
   else 
   {
      return index(Row, Col);
   }
}

QModelIndex colorModel::next(QModelIndex current)
{
   int idx = current.row() * 5 + current.column() + 1;
   if (idx <= MAXCOLORREF )
      return index(idx / 5, idx % 5); 
   return current;
}

QModelIndex colorModel::prev(QModelIndex current)
{
   int idx = current.row() * 5 + current.column() - 1;
   if( idx >= 0 )
      return index(idx / 5, idx % 5); 
   return current;
}

QModelIndex colorModel::last()
{
   int idx = MAXCOLORREF + 1;
   return index(idx / 5, idx % 5); 
}


//******************************************************************************************************************
// Custom Color ComboBox
//******************************************************************************************************************
cColorComboBox::cColorComboBox(QWidget *parent): QComboBox(parent) 
{
   STOPMAJ = false;

   QTableView* Table = new QTableView(this);
   Table->horizontalHeader()->hide();
   Table->verticalHeader()->hide();
   model =  new colorModel(this);
   setModel(model);
   setView(Table);

   for (int i = 0; i < 5; i++) 
   {
      Table->setColumnWidth(i,32);
      Table->resizeColumnToContents(i);
   }
   Table->resizeRowsToContents();
   ItemDelegate.ComboBox = this;
   setItemDelegate(&ItemDelegate);
   MakeIcons();
   int colWidth = Table->columnWidth(0);
   this->view()->setFixedWidth(5 * colWidth + 22);
   this->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
   //connect(Table, SIGNAL(itemSelectionChanged()), this, SLOT(s_ItemSelectionChanged()));
   //connect(Table, SIGNAL(cellPressed(int, int)), this, SLOT(s_ItemPressed(int, int)));
   //connect(this,SIGNAL(currentIndexChanged(int)),this,SLOT(currentIndexChanged(int)));
   connect(this,SIGNAL(activated(int)),this,SLOT(slotActivated(int)));
   connect(this,SIGNAL(currentIndexChanged(int)),this,SLOT(currentIndexChanged(int)));
   connect(Table,SIGNAL(pressed(const QModelIndex & )),this,SLOT(item_clicked(const QModelIndex &)) );
}

void cColorComboBox::SetCurrentColor(QString Color) 
{
   CurrentColor = Color;

   int i = 0;
   while ((i < MAXCOLORREF) && (ColorRef[i] != CurrentColor)) 
      i++;
   //int Row = i / 5;
   //int Col = i % 5;
   StandardColor = ((i >= 0) && (i < MAXCOLORREF));
   if (STOPMAJ) return;
   if (!StandardColor) 
   {
      SavedCustomColor = CurrentColor;
   }

   QModelIndex index = model->getIndexForColor(Color);
   if( index.isValid() )
   {
      blockSignals(true);
      ((QTableView *)view())->setCurrentIndex(index);
      setModelColumn(index.column());
      blockSignals(false);
      setCurrentIndex(index.row());
   }

   //if (!StandardColor) 
   //{
   //   SavedCustomColor = CurrentColor;
   //   //((QTableWidget *)view())->setCurrentCell(Row, 5);
   //   setCurrentIndex(Row);
   //}
   //else 
   //{
   //   //SavedCustomColor=ColorRef[i]/*"#fefefe"*/;
   //   ((QTableWidget *)view())->setCurrentCell(Row, Col);
   //   setCurrentIndex(Row);
   //}
   //MakeIcons();
}

//========================================================================================================================

QString cColorComboBox::GetCurrentColor() 
{
   return CurrentColor;
   //if (!CurrentColor) return SavedCustomColor;
   //int i = ((QTableWidget *)view())->currentRow() * 5 + ((QTableWidget *)view())->currentColumn();
   //StandardColor = ((i >= 0) && (i < MAXCOLORREF));
   //if ((i >= 0) && (i < MAXCOLORREF)) 
   //{
   //   *CurrentColor = ColorRef[i];
   //   return *CurrentColor;
   //}
   //return SavedCustomColor;
}

void cColorComboBox::keyPressEvent(QKeyEvent *event)
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
      setCurrentIndex(nextIndex.row());
      //CurrentFramingStyle = ((cFrameShapeTableItem*)(nextIndex.internalPointer()))->FrameStyle;
      //emit itemSelectionHaveChanged(GetCurrentFrameShape());
   }
}

void cColorComboBox::wheelEvent(QWheelEvent *e)
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
      setCurrentIndex(nextIndex.row());
   }
   e->accept();
}

void cColorComboBox::resizeEvent(QResizeEvent * /*ev*/)
{
   //qDebug() << "resize Combo from " << ev->oldSize() << " to " << ev->size();
}

//========================================================================================================================

void cColorComboBox::MakeIcons() 
{
   int ColorNum = 0;
   QModelIndex mIndex = ((QTableView *)view())->currentIndex();
   if( mIndex.isValid() )
      ColorNum = mIndex.row()*5 + mIndex.column();
   //if (CurrentColor) 
   //{
   //   while ((ColorNum < MAXCOLORREF) && (ColorRef[ColorNum] != *CurrentColor)) ColorNum++;
   //}
   //else 
   //{
   //   int CurrentRow = ((QTableWidget *)view())->currentRow();      if (CurrentRow < 0) CurrentRow = 0;
   //   int CurrentCol = ((QTableWidget *)view())->currentColumn();   if (CurrentCol < 0) CurrentCol = 0;
   //   ColorNum = CurrentRow * 5 + CurrentCol;
   //}
   //setIconSize(QSize(width()-16,16));
   setIconSize(QSize(24, 16));
   //setIconSize(QSize(24, 12));
   QPixmap  Image(iconSize());
   QPainter Painter;
   Painter.begin(&Image);
   if (ColorNum < MAXCOLORREF) 
   {
      Painter.fillRect(QRectF(0, 0, iconSize().width(), iconSize().height()), QColor(CurrentColor));
   }
   else 
   {
      Painter.fillRect(QRectF(0, 0, iconSize().width(), iconSize().height()), QColor(SavedCustomColor));
      QImage  Img(":/img/colorize.png");
      //Painter.drawImage(QRectF((iconSize().width()-16)/2,(iconSize().height()-16)/2,16,16),Img,QRectF(0,0,Img.width(),Img.height()));
      Painter.drawImage(QRectF(0, 0, 8, 8), Img, QRectF(0, 0, Img.width(), Img.height()));
   }
   Painter.end();
   setItemIcon(currentIndex(), QIcon(Image));
   setItemText(currentIndex(), "");
}

//========================================================================================================================

void cColorComboBox::s_ItemSelectionChanged() 
{
    //STOPMAJ=true;
    //QModelIndexList l = ((QTableWidget *)view())->selectedIndexes();
    //if( l.size() )
    //setCurrentIndex(((QTableWidget *)view())->currentRow());
    ////MakeIcons();
    ////emit currentIndexChanged(((QTableWidget *)view())->currentRow()*5+((QTableWidget *)view())->currentColumn());
    //STOPMAJ=false;
}

//========================================================================================================================

void cColorComboBox::s_ItemPressed(int,int) 
{
   int CurrentRow = ((QTableWidget *)view())->currentRow();      
   if (CurrentRow < 0) 
      CurrentRow = 0;
   int CurrentCol = ((QTableWidget *)view())->currentColumn();   
   if (CurrentCol < 0) 
      CurrentCol = 0;
   int ColorNum = CurrentRow * 5 + CurrentCol;
   if (ColorNum >= MAXCOLORREF)
   {
      // Open box to select custom color
      QColor color = QColorDialog::getColor(SavedCustomColor, 0, QString(), QColorDialog::ShowAlphaChannel);
      if (color.isValid())
      {
         //STOPMAJ = true;
         SavedCustomColor = color.name(QColor::HexArgb);
         setModelColumn(4);
         setCurrentIndex(-1);
         setCurrentIndex(((QTableWidget *)view())->currentRow());
         MakeIcons();
         emit currentIndexChanged(((QTableWidget *)view())->currentRow()*5 + ((QTableWidget *)view())->currentColumn());
         //STOPMAJ = false;
      }
   }
   else
   {
      setCurrentIndex(((QTableWidget *)view())->currentRow());
      MakeIcons();
      emit currentIndexChanged(((QTableWidget *)view())->currentRow()*5+((QTableWidget *)view())->currentColumn());
   }
}

//void cColorComboBox::setCurrentIndex(int index)
//{
//   int curRow = index / 5;
//   int curCol = index % 5;
//   ((QTableWidget *)view())->setCurrentCell(curRow, curCol);
//   //QModelIndex i = ((QTableWidget *)view())->model()->createIndex(curRow, curCol);
//   setModelColumn(curCol);
//   QComboBox::setCurrentIndex(index);
//}

//void cColorComboBox::currentIndexChanged(int index)
//{
//   //qDebug() << "currentIndexChanged to " << index;
//   QModelIndex idx = view()->currentIndex();
//   if( idx.isValid() )
//   {
//      //cFrameShapeTableItem* item = static_cast<cFrameShapeTableItem*>(idx.internalPointer());
//      //if( !dontEmit )
//      //{
//      //   CurrentFramingStyle = item->FrameStyle;
//      //   emit itemSelectionHaveChanged(CurrentFramingStyle);
//      //}
//   }
//   int curRow = index / 5;
//   int curCol = index % 5;
//   ((QTableWidget *)view())->setCurrentCell(curRow, curCol);
//   //QModelIndex i = ((QTableWidget *)view())->model()->createIndex(curRow, curCol);
//   setModelColumn(curCol);
//   MakeIcons();
//   //QComboBox::currentIndexChanged(index);
//}
//
void cColorComboBox::currentIndexChanged(int index)
{
   QModelIndex idx = view()->currentIndex();
   //CurrentFramingStyle = -1;
   if( idx.isValid() )
   {

      int i = idx.row()*5+idx.column();
      if( i < MAXCOLORREF )
         CurrentColor = ColorRef[i];
      else
         CurrentColor = SavedCustomColor;
      //cFrameShapeTableItem* item = static_cast<cFrameShapeTableItem*>(idx.internalPointer());
      //if( !dontEmit )
      //{
      //   CurrentFramingStyle = item->FrameStyle;
      //   emit itemSelectionHaveChanged(CurrentFramingStyle);
      //}
   }
   //emit QComboBox::currentIndexChanged(index);
}


void cColorComboBox::slotActivated(int /*index*/)
{
   QModelIndex idx = view()->currentIndex();
   if (!idx.isValid())
      return;
   setModelColumn(idx.column());
}

void cColorComboBox::item_clicked(const QModelIndex &mi)
{
   s_ItemPressed(mi.row(), mi.column());
}