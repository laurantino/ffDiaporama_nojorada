/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>

#include <math.h>
#include <stdlib.h>

#include "transitionsplugin.h"
#include <qdebug.h>

QStringList TransitionPlugin::transitions() const
{
    return QStringList() << tr("Flip Horizontally") << tr("Flip Vertically")
                         << tr("Smudge...") << tr("Threshold...");
}

//void TransitionPlugin::doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight)
//{
//   if (TransitionSubType == 0)
//   {
//      const int numStripes = 10;
//      int stripeWidth = DestImageWith / numStripes;
//      int fullStripes = PCT * 10;
//      WorkingPainter->drawImage(0,0,*ImageA);
//      if (fullStripes)
//      {
//         WorkingPainter->drawImage(0, 0, *ImageB, 0, 0, fullStripes*stripeWidth, DestImageHeight);
//      }
//      if (fullStripes < numStripes)
//      {
//         int xPos = fullStripes*stripeWidth;
//         int destHeight = ((PCT - ((double)fullStripes/10.0)) * 10.0) * DestImageHeight;
//         int destY = 0 - (DestImageHeight - destHeight);
//         WorkingPainter->drawImage(xPos, destY, *ImageB, xPos, 0, stripeWidth, DestImageHeight/* destHeight*/);
//      }
//   }
//}

void TransitionPlugin::doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight)
{
   int numStripes = 15;
   double diffPCT = 0.10; // % Differenz
   if (TransitionSubType == 0)
   {
      double diffSum = numStripes * diffPCT;
      int diffPix = DestImageHeight * (1 + diffSum);
      int stripeWidth = DestImageWith / numStripes;
      int stripe = 0;
      WorkingPainter->drawImage(0, 0, *ImageA);
      while (stripe < numStripes)
      {
         int xPos = stripe*stripeWidth;
         int destHeight = diffPix * PCT - (DestImageHeight*diffPCT*stripe);
         int destY = qMin(0, 0 - (DestImageHeight - destHeight));
         //qDebug() << "Destheight " << destHeight;
         if (destHeight > 0)
            WorkingPainter->drawImage(xPos, 0, *ImageB, xPos, 0, stripeWidth, destHeight);
         stripe++;
      }
      //qDebug() << " ";
   }
   else if (TransitionSubType == 1)
   {
      double diffSum = numStripes * diffPCT;
      int diffPix = DestImageHeight * (1 + diffSum);
      int stripeWidth = DestImageWith / numStripes;
      int stripe = 0;
      WorkingPainter->drawImage(0, 0, *ImageA);
      while (stripe < numStripes)
      {
         int xPos = stripe*stripeWidth;
         int destHeight = diffPix * PCT - (DestImageHeight*diffPCT*stripe);
         int destY = qMin(0, 0 - (DestImageHeight - destHeight));
         WorkingPainter->drawImage(xPos, destY, *ImageB, xPos, 0, stripeWidth, DestImageHeight/* destHeight*/);
         stripe++;
      }
   }
   else if (TransitionSubType == 2)
   {
      double diffSum = numStripes * diffPCT;
      int diffPix = DestImageHeight * (1 + diffSum);
      int marker = -((DestImageHeight + diffPix)*PCT);
      int stripeWidth = DestImageWith / numStripes;
      int stripe = 0;
      int yposA = 0 + diffPix * PCT;
      int yposB = yposA - DestImageHeight;
      int deltaY = DestImageHeight * diffPCT;
      //WorkingPainter->drawImage(0,0, *ImageA);
      while (stripe < numStripes)
      {
         int xPos = stripe*stripeWidth;
         QRectF srcRect(xPos, 0, stripeWidth, DestImageHeight);
         QRectF dstRectA(xPos, yposA < 0 ? 0 : yposA, stripeWidth, DestImageHeight);
         yposB = yposA - DestImageHeight + 1;
         QRectF dstRectB(xPos, yposB > 0 ? 0 : yposB, stripeWidth, DestImageHeight);
         if (dstRectA.top() < 0)
            dstRectA.moveTop(0);
         WorkingPainter->drawImage(dstRectA, *ImageA, srcRect);
         //qDebug() << "marker " << marker << "dstRectA " << dstRectA;
         WorkingPainter->drawImage(dstRectB, *ImageB, srcRect);
         marker -= DestImageHeight*diffPCT;
         yposA -= deltaY;
         stripe++;
      }
   }
   else if (TransitionSubType >= 3 && TransitionSubType <= 5)
   {
      //numStripes = 6;
//      double diffSum = int(numStripes * diffPCT);
//      int diffPix = int(DestImageHeight * (1 + diffSum));
      //int marker = -((DestImageHeight + diffPix)*PCT);
      int stripeWidth = DestImageWith / numStripes;
//      int yposA = int(0 + diffPix * PCT);
//      int yposB = yposA - DestImageHeight;
//      int deltaY = int(DestImageHeight * diffPCT);
      WorkingPainter->drawImage(0,0, *ImageB);
//      double stripeP = 1.0 / numStripes;
      int stripe = 0;
      qreal rotation = 90 * PCT;
      while (stripe < numStripes)
      {
         int xPos = stripe*stripeWidth;
         QRectF srcRect(xPos, 0, stripeWidth, DestImageHeight);
         QRectF dstRect(xPos, 0, stripeWidth, DestImageHeight);
         //qreal rotation = 90/stripeP * (PCT-stripeP*stripe);
//         qDebug() << "stripe " << stripe << " winkel " << rotation;
         if (rotation < 90)
         {
            if (rotation > 0)
            {
               QTransform  Matrix;
               int rPos = xPos;
               if (TransitionSubType > 3)
                  rPos += +stripeWidth / 2;
               if (TransitionSubType > 4)
                  rPos += +stripeWidth;
               Matrix.translate(rPos,0);
               Matrix.rotate(-rotation, Qt::YAxis);     // Standard axis
               Matrix.translate(-(rPos), 0);
               WorkingPainter->save();
               WorkingPainter->setWorldTransform(Matrix, false);
               WorkingPainter->drawImage(dstRect, *ImageA, srcRect);
               WorkingPainter->restore();
            }
            else
               WorkingPainter->drawImage(dstRect, *ImageA, srcRect);
         }
         stripe++;
      }
   }
   else if (TransitionSubType == 6)
   {
      double diffSum = numStripes * diffPCT;
      int diffPix = int(DestImageHeight * (1 + diffSum));
      int stripeWidth = DestImageWith / numStripes;
      int stripe = 0;
      WorkingPainter->drawImage(0, 0, *ImageA);
      QRectF srcRect(0, 0, stripeWidth, DestImageHeight);
      QRectF dstRect(0, 0, stripeWidth, DestImageHeight);
      while (stripe < numStripes)
      {
         int xPos = stripe*stripeWidth;
         int destHeight = int(diffPix * PCT - (DestImageHeight*diffPCT*stripe));
         int destY = qMin(0, 0 - (DestImageHeight - destHeight));
         int vY = DestImageHeight + destY;
         if (vY > 0)
         {
            srcRect.moveLeft(xPos);
            srcRect.moveTop(DestImageHeight / 2 - destHeight / 2);
            srcRect.setHeight(destHeight);
            dstRect.moveLeft(xPos);
            dstRect.moveTop(DestImageHeight / 2 - destHeight / 2);
            dstRect.setHeight(destHeight);
            //qDebug() << "destY " << destY << " destHeight " << destHeight << " vY " << vY << " srcRect " << srcRect << " dstRect " << dstRect;
            //WorkingPainter->drawImage(xPos, destY, *ImageB, xPos, 0, stripeWidth, DestImageHeight/* destHeight*/);
            WorkingPainter->drawImage(dstRect, *ImageB, srcRect);
         }
         stripe++;
      }
   }
   else if (TransitionSubType == 7)
   {
      double diffSum = numStripes * diffPCT;
      int diffPix = int(DestImageHeight * (1 + diffSum));
      int stripeWidth = DestImageWith / numStripes;
      int stripe = 0;
      WorkingPainter->drawImage(0, 0, *ImageA);
      QRectF srcRect(0, 0, stripeWidth, DestImageHeight);
      QRectF dstRect(0, 0, stripeWidth, DestImageHeight);
      while (stripe < numStripes)
      {
         int xPos = stripe*stripeWidth;
         int destHeight = int(diffPix * PCT - (DestImageHeight*diffPCT*stripe));
         int destY = qMin(0, 0 - (DestImageHeight - destHeight));
         int vY = DestImageHeight + destY;
         if (vY > 0)
         {
            srcRect.moveLeft(xPos);
            int dPosY = DestImageHeight / 2 - destHeight / 2;
            if (dPosY < 0)
               dPosY = 0;
            srcRect.moveTop(dPosY);
            //srcRect.setHeight(destHeight);
            dstRect.moveLeft(xPos);
            dstRect.moveTop(dPosY);
            dstRect.setHeight(destHeight < DestImageHeight ? destHeight : DestImageHeight);
//            qDebug() << "destY " << destY << " destHeight " << destHeight << " vY " << vY << " srcRect " << srcRect << " dstRect " << dstRect;
            //WorkingPainter->drawImage(xPos, destY, *ImageB, xPos, 0, stripeWidth, DestImageHeight/* destHeight*/);
            WorkingPainter->drawImage(dstRect, *ImageB, srcRect);
         }
         stripe++;
      }
   }
}

QImage TransitionPlugin::getIcon(int TransitionSubType) const
{
    Q_UNUSED(TransitionSubType);
   return QImage();
}
