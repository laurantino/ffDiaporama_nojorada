#ifndef FFDINTERFACES_H
#define FFDINTERFACES_H

#include <QtPlugin>

//QT_BEGIN_NAMESPACE
class QImage;
class QPainter;
//class QWidget;
//class QPainterPath;
//class QPoint;
//class QRect;
//class QString;
//class QStringList;
//QT_END_NAMESPACE
//

//class BrushInterface
//{
//public:
//    virtual ~BrushInterface() {}
//
//    virtual QStringList brushes() const = 0;
//    virtual QRect mousePress(const QString &brush, QPainter &painter,
//                             const QPoint &pos) = 0;
//    virtual QRect mouseMove(const QString &brush, QPainter &painter,
//                            const QPoint &oldPos, const QPoint &newPos) = 0;
//    virtual QRect mouseRelease(const QString &brush, QPainter &painter,
//                               const QPoint &pos) = 0;
//};

//class ShapeInterface
//{
//public:
//    virtual ~ShapeInterface() {}
//
//    virtual QStringList shapes() const = 0;
//    virtual QPainterPath generateShape(const QString &shape,
//                                       QWidget *parent) = 0;
//};

//class FilterInterface
//{
//public:
//    virtual ~FilterInterface() {}
//
//    virtual QStringList filters() const = 0;
//    virtual QImage filterImage(const QString &filter, const QImage &image,
//                               QWidget *parent) = 0;
//};

class ffdPlugin
{
   public:
   virtual ~ffdPlugin() {}
   virtual QString pluginName() const = 0;
};

class TransitionInterface : public ffdPlugin
{
   protected:
      int iFamilyID;
   public:
      virtual ~TransitionInterface() { iFamilyID = -1; }

      virtual QString familyName() const = 0;
      void setFamilyID(int i) { iFamilyID = i; }
      virtual int familyID() const = 0;
      virtual int numTransitions() const = 0;
      virtual QStringList transitions() const = 0;
      virtual void doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight) = 0;
      virtual QImage getIcon(int TransitionSubType) const = 0;
};

class FilterInterface : public ffdPlugin
{
   public:
      virtual ~FilterInterface() {}

      virtual QStringList filters() const = 0;
      virtual QImage filterImage(const QString &filter, const QImage &image, QWidget *parent) = 0;
};

//#define BrushInterface_iid "org.qt-project.Qt.Examples.PlugAndPaint.BrushInterface"
//Q_DECLARE_INTERFACE(BrushInterface, BrushInterface_iid)
//#define ShapeInterface_iid  "org.qt-project.Qt.Examples.PlugAndPaint.ShapeInterface"
//Q_DECLARE_INTERFACE(ShapeInterface, ShapeInterface_iid)
//
#define TransitionInterface_iid "org.ffDiaporama.TransitionInterface"
Q_DECLARE_INTERFACE(TransitionInterface, TransitionInterface_iid)


#endif //FFDINTERFACES_H
