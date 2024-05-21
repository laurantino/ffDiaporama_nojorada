#ifndef __GlowFilter__
#define __GlowFilter__
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

#include "image/GaussianFilter.h"
/**
 * A filter which adds Gaussian blur to an image, producing a glowing effect.
 * @author Jerry Huxtable
 */
class GlowFilter : public GaussianFilter 
{

	private:
    float amount;
	
	public:
    GlowFilter() {
      amount = 0.5f;
		radius = 2;
	}
	
	/**
	 * Set the amount of glow.
	 * @param amount the amount
     * @min-value 0
     * @max-value 1
     * @see #getAmount
	 */
	void setAmount( float amount ) {
		this->amount = amount;
	}
	
	/**
	 * Get the amount of glow.
	 * @return the amount
     * @see #setAmount
	 */
	float getAmount() {
		return amount;
	}
	
   QImage *filter( const QImage &src, QImage *dst ) 
   {
        int width = src.width();
        int height = src.height();

        if ( dst == 0 )
            dst = createCompatibleDestImage( src );

        int *inPixels = new int[width*height];
        int *outPixels = new int[width*height];
        memcpy(inPixels,(const int *)src.bits(),width*height*4);

		if ( radius > 0 ) {
			convolveAndTranspose(kernel, inPixels, outPixels, width, height, alpha, alpha && premultiplyAlpha, false, CLAMP_EDGES);
			convolveAndTranspose(kernel, outPixels, inPixels, height, width, alpha, false, alpha && premultiplyAlpha, CLAMP_EDGES);
		}

        //src.getRGB( 0, 0, width, height, outPixels, 0, width );
        memcpy(outPixels,(const int *)src.bits(),width*height*4);

		float a = 4*amount;

		int index = 0;
		for ( int y = 0; y < height; y++ ) {
			for ( int x = 0; x < width; x++ ) {
				int rgb1 = outPixels[index];
				int r1 = (rgb1 >> 16) & 0xff;
				int g1 = (rgb1 >> 8) & 0xff;
				int b1 = rgb1 & 0xff;

				int rgb2 = inPixels[index];
				int r2 = (rgb2 >> 16) & 0xff;
				int g2 = (rgb2 >> 8) & 0xff;
				int b2 = rgb2 & 0xff;

				r1 = PixelUtils::clamp( (int)(r1 + a * r2) );
				g1 = PixelUtils::clamp( (int)(g1 + a * g2) );
				b1 = PixelUtils::clamp( (int)(b1 + a * b2) );

				inPixels[index] = (rgb1 & 0xff000000) | (r1 << 16) | (g1 << 8) | b1;
				index++;
			}
		}

        //dst.setRGB( 0, 0, width, height, inPixels, 0, width );
        memcpy(dst->bits(), inPixels,width*height*4);

        delete [] inPixels;
        delete [] outPixels;
        return dst;
    }

	//public String toString() {
	//	return "Blur/Glow...";
	//}
};
#endif // __GlowFilter__

