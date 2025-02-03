#ifndef __QT_CUDA__
#define __QT_CUDA__

#include <cuda_runtime.h>      // CUDA Runtime
#include <npp.h>               // CUDA NPP Definitions
//#include <helper_cuda.h>       // helper for CUDA Error handling and initialization
#include <QImage>
extern bool use_cuda;
typedef struct cudaImage
{
   Npp8u *pMem;
   int pitch;
   int width;
   int height;
   QImage::Format format;
   cudaImage() { pMem = NULL; pitch = 0; width = 0; height = 0; format = QImage::Format_Invalid;}
   //NppStatus error;
   //public:
   //   cudaImage();
   //   cudaImage expandCanvas( int width, int height );
   //   cudaImage rotate();
   //   cudaImage take();
   //   cudaImage scale();
} cudaImage;

class cQtCuda
{
   int dev;
   bool initialised;

   bool usableImageFormat(QImage::Format);
   int bpp(QImage::Format);
   Npp8u *deviceMalloc(QImage::Format,int nWidthPixels, int nHeightPixels, int * pStepBytes);
   NppStatus nppiResizeSqrPixel_8u(QImage::Format,const Npp8u *pSrc, NppiSize oSrcSize, 
         int nSrcStep, NppiRect oSrcROI, Npp8u *pDst, int nDstStep, NppiRect oDstROI, 
         double nXFactor, double nYFactor, double nXShift, double nYShift, int eInterpolation);
   NppStatus nppiCopyConstBorder_8u(QImage::Format format, const Npp8u * pSrc,   int nSrcStep, NppiSize oSrcSizeROI,
                                     Npp8u * pDst,   int nDstStep, NppiSize oDstSizeROI,
                                     int nTopBorderHeight, int nLeftBorderWidth,
                                     const Npp8u aValue[]);
   NppStatus nppiRotate_8u(QImage::Format format, const Npp8u *pSrc, NppiSize oSrcSize, int nSrcStep, NppiRect oSrcROI, 
                        Npp8u * pDst, int nDstStep, NppiRect oDstROI,
                        double nAngle, double nShiftX, double nShiftY, int eInterpolation);
   public:
      cQtCuda();
      ~cQtCuda() { exitCuda(); }
      bool initCuda();
      bool isOk() { return use_cuda && initialised; }
      void exitCuda();
      static cQtCuda *getCuda();

      Npp8u *image2device(const QImage & image, int *Pitch);
      QImage scaled(const QImage & image, const QSize & size, Qt::AspectRatioMode aspectRatioMode = Qt::IgnoreAspectRatio, Qt::TransformationMode transformMode = Qt::FastTransformation);
      QImage scaled(const QImage & image, int width, int height, Qt::AspectRatioMode aspectRatioMode = Qt::IgnoreAspectRatio, Qt::TransformationMode transformMode = Qt::FastTransformation);
      QImage scaledToHeight(const QImage & image, int height, Qt::TransformationMode mode = Qt::FastTransformation);
      QImage scaledToWidth(const QImage & image, int width, Qt::TransformationMode mode = Qt::FastTransformation);
      void Transition_Luma(QImage Luma,double PCT,QImage *ImageA,QImage *ImageB,QPainter *WorkingPainter,int DestImageWith,int DestImageHeight);

      QImage expandCanvas(const QImage & image, int newWidth, int newHeight);
      QImage expandCanvasCopyScale(const QImage & image, int newWidth, int newHeight, int posX, int posY, int w, int h, int scaledw, int scaledh, Qt::TransformationMode transformMode = Qt::FastTransformation);
      //QImage expandCanvasRotateCopyScale(const QImage & image, int newWidth, int newHeight, double rotationFactor, int posX, int posY, int w, int h, int scaledw, int scaledh, Qt::TransformationMode transformMode = Qt::FastTransformation);
      QImage expandCanvasRotateCopyScale_old(const QImage & image, int newWidth, int newHeight, double rotationFactor, int posX, int posY, int w, int h, int scaledw, int scaledh, Qt::TransformationMode transformMode);
      QImage expandCanvasRotateCopyScale(const QImage & image, int newWidth, int newHeight, double rotationFactor, int posX, int posY, int w, int h, int scaledw, int scaledh, Qt::TransformationMode transformMode);

      cudaImage image2device(const QImage & image);
      cudaImage expandCanvas(const cudaImage& src, int newWidth, int newHeight);
      cudaImage rotate(const cudaImage& src, double rotationFactor);
      cudaImage take(const cudaImage& src, int posX, int posY, int w, int h);
      cudaImage scale(const cudaImage& src, int scaledw, int scaledh, Qt::TransformationMode transformMode);
      bool toARGB32_Premultiplied(cudaImage& src);
      QImage fromcudaImage(const cudaImage& src);
};
#endif // __QT_CUDA__
