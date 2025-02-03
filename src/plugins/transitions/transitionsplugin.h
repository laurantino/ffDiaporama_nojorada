#ifndef TRANSITIONSPLUGIN_H
#define TRANSITIONSPLUGIN_H

#include <ffd_interfaces.h>

#include <QObject>
#include <QtPlugin>
#include <QStringList>
#include <QImage>

class TransitionPlugin : public QObject, public TransitionInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.ffDiaporama.TransitionInterface" FILE "transitions.json")
    Q_INTERFACES(TransitionInterface)

public:
   QString pluginName() const { return "TransitionPlugin Stripes"; }
   QString familyName() const { return "Stripes"; }
   int familyID() const override { return iFamilyID; }
   int numTransitions() const override { return 8; };
   QStringList transitions() const override;
   void doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight) override;
   QImage getIcon(int TransitionSubType) const override;
};

#endif //TRANSITIONSPLUGIN_H
