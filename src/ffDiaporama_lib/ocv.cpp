#ifdef WITH_OPENCV
#include "ocv.h"

void OCVGaussianBlur(QImage &img, float radius, float sigma)
{
   if (sigma == 0.0) 
      return;

//   if (Img.format()!=QImage::Format_ARGB32) Img=Img.convertToFormat(QImage::Format_ARGB32);

   // figure out optimal kernel width
   int kern_width;
   if (radius > 0) 
      kern_width = (int)(2*ceil(radius)+1);
   else
      kern_width = 3;

   cv::Mat m = ASM::QImageToCvMat(img, false);
   cv::GaussianBlur( m, m, cv::Size(kern_width,kern_width), sigma/*,double sigmaY = 0, int borderType = BORDER_DEFAULT */);
}

QImage OCVScale(QImage img, int dstWidth, int dstHeight)
{
   cv::Mat m = ASM::QImageToCvMat(img, false);
   cv::Mat  dstmat( dstHeight, dstWidth, m.type() );
   resize(m, dstmat, dstmat.size(), 0, 0, CV_INTER_AREA);
   QImage rimg = ASM::cvMatToQImage(dstmat);
   return rimg.convertToFormat(QImage::Format_ARGB32_Premultiplied);
}

#endif
