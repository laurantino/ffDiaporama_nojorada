#ifndef __WholeImageFilter__
#define __WholeImageFilter__
/*
Copyright 2006 Jerry Huxtable

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "AbstractBufferedImageOp.h"

/**
 * A filter which acts as a superclass for filters which need to have the whole image in memory
 * to do their stuff.
 */
class WholeImageFilter : public AbstractBufferedImageOp 
{

	/**
     * The output image bounds.
     */
   protected:
      QRect transformedSpace;

	/**
     * The input image bounds.
     */
	   QRect originalSpace;
	
	/**
	 * Construct a WholeImageFilter.
	 */
	WholeImageFilter() 
   {
	}

   public:
   QImage *filter(const QImage &src, QImage *dst ) 
   {
      int width = src.width();
      int height = src.height();
		//int type = src.getType();
		//WritableRaster srcRaster = src.getRaster();

		originalSpace = QRect(0, 0, width, height);
		transformedSpace = QRect(0, 0, width, height);
		transformSpace(transformedSpace);

      if ( dst == 0) 
         dst = createCompatibleDestImage(src);

		const int *inPixels = (const int *)src.bits();//getRGB( src, 0, 0, width, height, null );
		int *outPixels = filterPixels( width, height, inPixels, transformedSpace );
      memcpy(dst->bits(), outPixels, dst->QT_IMAGESIZE_IN_BYTES());
      delete outPixels;

        return dst;
    }

	/**
     * Calculate output bounds for given input bounds.
     * @param rect input and output rectangle
     */
	void transformSpace(QRect /*rect*/) { }
	
	/**
     * Actually filter the pixels.
     * @param width the image width
     * @param height the image height
     * @param inPixels the image pixels
     * @param transformedSpace the output bounds
     * @return the output pixels
     */
	virtual int *filterPixels( int width, int height, const int *inPixels, QRect transformedSpace )=0;
};

#endif // __WholeImageFilter__
