#ifndef __TransformFilter__
#define __TransformFilter__
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

#include "image/AbstractBufferedImageOp.h"

/**
 * An abstract superclass for filters which distort images in some way. The subclass only needs to override
 * two methods to provide the mapping between source and destination pixels.
 */
class TransformFilter : public AbstractBufferedImageOp 
{
   public:
    /**
     * Treat pixels off the edge as zero.
     */
	static const int ZERO = 0;

    /**
     * Clamp pixels to the image edges.
     */
	static const int CLAMP = 1;

    /**
     * Wrap pixels off the edge onto the oppsoite edge.
     */
	static const int WRAP = 2;

    /**
     * Clamp pixels RGB to the image edges, but zero the alpha. This prevents gray borders on your image.
     */
	static const int RGB_CLAMP = 3;

    /**
     * Use nearest-neighbout interpolation.
     */
	static const int NEAREST_NEIGHBOUR = 0;

    /**
     * Use bilinear interpolation.
     */
	static const int BILINEAR = 1;

    /**
     * The action to take for pixels off the image edge.
     */
	protected:
      int edgeAction;

    /**
     * The type of interpolation to use.
     */
	   int interpolation;

    /**
     * The output image rectangle.
     */
	QRect transformedSpace;

    /**
     * The input image rectangle.
     */
	QRect originalSpace;

    /**
     * Set the action to perform for pixels off the edge of the image.
     * @param edgeAction one of ZERO, CLAMP or WRAP
     * @see #getEdgeAction
     */
	public:
   TransformFilter() : AbstractBufferedImageOp()
   {
      edgeAction = RGB_CLAMP;
      interpolation = BILINEAR;
   }
    void setEdgeAction(int edgeAction) {
		this->edgeAction = edgeAction;
	}

    /**
     * Get the action to perform for pixels off the edge of the image.
     * @return one of ZERO, CLAMP or WRAP
     * @see #setEdgeAction
     */
	int getEdgeAction() {
		return edgeAction;
	}
	
    /**
     * Set the type of interpolation to perform.
     * @param interpolation one of NEAREST_NEIGHBOUR or BILINEAR
     * @see #getInterpolation
     */
	void setInterpolation(int interpolation) {
		this->interpolation = interpolation;
	}

    /**
     * Get the type of interpolation to perform.
     * @return one of NEAREST_NEIGHBOUR or BILINEAR
     * @see #setInterpolation
     */
	int getInterpolation() {
		return interpolation;
	}
	
    /**
     * Inverse transform a point. This method needs to be overriden by all subclasses.
     * @param x the X position of the pixel in the output image
     * @param y the Y position of the pixel in the output image
     * @param out the position of the pixel in the input image
     */
	protected:
      virtual void transformInverse(int x, int y, float out[2])=0;

    /**
     * Forward transform a rectangle. Used to determine the size of the output image.
     * @param rect the rectangle to transform
     */
	void transformSpace(QRect rect) {}

    public:
     QImage *filter(const QImage &src, QImage *dst ) 
     {
        int width = src.width();
        int height = src.height();
        //int type = src->format();
        //WritableRaster srcRaster = src.getRaster();

        originalSpace = QRect(0, 0, width, height);
        transformedSpace = QRect(0, 0, width, height);
        transformSpace(transformedSpace);

        if ( dst == 0 ) 
           dst = createCompatibleDestImage(src);
        dst->fill(Qt::blue);
        const int *inPixels = (const int *)src.bits();

        if ( interpolation == NEAREST_NEIGHBOUR )
           return filterPixelsNN( dst, width, height, inPixels, transformedSpace );

        int srcWidth = width;
        int srcHeight = height;
        int srcWidth1 = width-1;
        int srcHeight1 = height-1;
        int outWidth = transformedSpace.width();
        int outHeight = transformedSpace.height();
        int outX, outY;
        //int index = 0;
        int *outPixels = new int[outWidth];
        int *destPixels = (int *)dst->bits();

        outX = transformedSpace.left();
        outY = transformedSpace.top();
        float out[2];

        for (int y = 0; y < outHeight; y++) 
        {
           for (int x = 0; x < outWidth; x++) 
           {
              transformInverse(outX+x, outY+y, out);
              int srcX = (int)floor( out[0] );
              int srcY = (int)floor( out[1] );
              float xWeight = out[0]-srcX;
              float yWeight = out[1]-srcY;
              int nw, ne, sw, se;

              if ( srcX >= 0 && srcX < srcWidth1 && srcY >= 0 && srcY < srcHeight1) 
              {
                 // Easy case, all corners are in the image
                 int i = srcWidth*srcY + srcX;
                 nw = inPixels[i];
                 ne = inPixels[i+1];
                 sw = inPixels[i+srcWidth];
                 se = inPixels[i+srcWidth+1];
              } 
              else 
              {
                 // Some of the corners are off the image
                 nw = getPixel( inPixels, srcX, srcY, srcWidth, srcHeight );
                 ne = getPixel( inPixels, srcX+1, srcY, srcWidth, srcHeight );
                 sw = getPixel( inPixels, srcX, srcY+1, srcWidth, srcHeight );
                 se = getPixel( inPixels, srcX+1, srcY+1, srcWidth, srcHeight );
              }
              outPixels[x] = ImageMath::bilinearInterpolate(xWeight, yWeight, nw, ne, sw, se);
           }
           //setRGB( dst, 0, y, transformedSpace.width(), 1, outPixels );
           memcpy(destPixels+y*width, outPixels,transformedSpace.width()*4); // ???
        }
        delete [] outPixels;
        return dst;
     }

	private:
    int getPixel( const int *pixels, int x, int y, int width, int height ) 
    {
		if (x < 0 || x >= width || y < 0 || y >= height) 
      {
			switch (edgeAction) {
			case ZERO:
			default:
				return 0;
			case WRAP:
				return pixels[(ImageMath::mod(y, height) * width) + ImageMath::mod(x, width)];
			case CLAMP:
				return pixels[(ImageMath::clamp(y, 0, height-1) * width) + ImageMath::clamp(x, 0, width-1)];
			case RGB_CLAMP:
				return pixels[(ImageMath::clamp(y, 0, height-1) * width) + ImageMath::clamp(x, 0, width-1)] & 0x00ffffff;
			}
		}
		return pixels[ y*width+x ];
	}

	protected: 
      QImage *filterPixelsNN( QImage *dst, int width, int height, const int *inPixels, QRect transformedSpace ) 
      {
         int srcWidth = width;
         int srcHeight = height;
         int outWidth = transformedSpace.width();
         int outHeight = transformedSpace.height();
         int outX, outY, srcX, srcY;
         int *outPixels = new int[outWidth];

         outX = transformedSpace.left();
         outY = transformedSpace.top();
         int rgb[4];
         float out[2];

         for (int y = 0; y < outHeight; y++) 
         {
            for (int x = 0; x < outWidth; x++) 
            {
               transformInverse(outX+x, outY+y, out);
               srcX = (int)out[0];
               srcY = (int)out[1];
               // int casting rounds towards zero, so we check out[0] < 0, not srcX < 0
               if (out[0] < 0 || srcX >= srcWidth || out[1] < 0 || srcY >= srcHeight) 
               {
                  int p;
                  switch (edgeAction) 
                  {
                     case ZERO:
                     default:
                        p = 0;
                        break;
                     case WRAP:
                        p = inPixels[(ImageMath::mod(srcY, srcHeight) * srcWidth) + ImageMath::mod(srcX, srcWidth)];
                        break;
                     case CLAMP:
                        p = inPixels[(ImageMath::clamp(srcY, 0, srcHeight-1) * srcWidth) + ImageMath::clamp(srcX, 0, srcWidth-1)];
                        break;
                     case RGB_CLAMP:
                        p = inPixels[(ImageMath::clamp(srcY, 0, srcHeight-1) * srcWidth) + ImageMath::clamp(srcX, 0, srcWidth-1)] & 0x00ffffff;
                  }
                  outPixels[x] = p;
               } 
               else 
               {
                  int i = srcWidth*srcY + srcX;
                  rgb[0] = inPixels[i];
                  outPixels[x] = inPixels[i];
               }
            }
            //setRGB( dst, 0, y, transformedSpace.width(), 1, outPixels );
            memcpy(dst+y*width, outPixels,transformedSpace.width()); // ???
         }
         return dst;
      }

};

#endif // __TransformFilter__
