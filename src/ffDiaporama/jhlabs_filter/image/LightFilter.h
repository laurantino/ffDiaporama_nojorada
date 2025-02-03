#ifndef __LightFilter__
#define __LightFilter__
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

#include "WholeImageFilter.h"
#include "../math/ImageFunction2D.h"
#include "../vecmath/Vector3f.h"
#include "../vecmath/Tuple4f.h"
#include "../vecmath/Color4f.h"
#include "ConvolveFilter.h" // for Kernel
#include "ImageMath.h"
#include "GaussianFilter.h"
#include <QVector3D>
#include <QVector4D>
#include <QtMath>
#include <QColor>

//typedef QVector3D Color3f;
////typedef QVector4D Color4f;
//class Color4f : public QVector4D
//{
//   public:
//      Color4f()
//      {
//      }
//      Color4f(QColor c)
//      {
//         set(c);
//      }
//      void scale(float s)
//      {
//         setX(x() * s);
//         setY(y() * s);
//         setZ(z() * s);
//         setW(w() * s);
//         //this->
//      }
//      void set(QColor c)
//      {
//         QRgb rgb(c.rgba());
//         setX(qRed(rgb));
//         setY(qGreen(rgb));
//         setZ(qBlue(rgb));
//         setW(qAlpha(rgb));
//      }
//};

// todo (fromn java3d):
    /**
     * A class representing material properties.
     */
class Material 
{
	public:
	int diffuseColor;
	int specularColor;
	float ambientIntensity;
	float diffuseReflectivity;
	float specularReflectivity;
	float highlight;
	float reflectivity;
	float opacity;


   /*
   lighting enable : true
ambient color : (0.2, 0.2, 0.2)
emmisive color : (0.0, 0.0, 0.0)
diffuse color : (1.0, 1.0, 1.0)
specular color : (1.0, 1.0, 1.0)
shininess : 64
color target : DIFFUSE
*/
   public:
   Material() 
   {
		ambientIntensity = 0.9f;
		diffuseReflectivity = 0.0f;
		specularReflectivity = 1.0f;
		highlight = 1.3f;
		reflectivity = 0.0f;
		diffuseColor = 0xffffffff;
		specularColor = 0xffffffff;
      opacity = 1.0;
	}

   void setAmbientIntensity(double d)
   {
      ambientIntensity = d;
   }
	void setDiffuseColor(int diffuseColor) {
		this->diffuseColor = diffuseColor;
	}

	int getDiffuseColor() {
		return diffuseColor;
	}

	void setOpacity( float opacity ) {
		this->opacity = opacity;
	}

	float getOpacity() {
		return opacity;
	}

};

static const int eAMBIENT = 0;
static const int eDISTANT = 1;
static const int ePOINT = 2;
static const int eSPOT = 3;

//#define PI 3.14159265358979323846
    /**
     * A class representing a light.
     */
class Light 
{
   public:
		int type;
		Vector3f position;
		Vector3f direction;
		Color4f realColor;
		Color4f color;
		float intensity;
		float azimuth;
		float elevation;
		float focus;
		float centreX, centreY;
		float coneAngle;
		float cosConeAngle;
		float distance;

		public:
      Light() 
      {
         init();
			azimuth = 270*ImageMath::PI/180.0f;
			//elevation = 0.5235987755982988f;
         elevation = 0.25;
			intensity = 1.0f;
		}
		
		Light(float azimuth, float elevation, float intensity) 
      {
         init();
			this->azimuth = azimuth;
			this->elevation = elevation;
			this->intensity = intensity;
		}

      void init()
      {
         type = eAMBIENT;
         color = 0xffffffff;
         focus = 0.5f;
         centreX = 0.5f;
         centreY = 0.5f;
         coneAngle = ImageMath::PI/6;
         distance = 1000.0f;
      }
		
		void setAzimuth(float azimuth) {
			this->azimuth = azimuth;
		}

		float getAzimuth() {
			return azimuth;
		}

		void setElevation(float elevation) {
			this->elevation = elevation;
		}

		float getElevation() {
			return elevation;
		}

		void setDistance(float distance) {
			this->distance = distance;
		}

		float getDistance() {
			return distance;
		}

		void setIntensity(float intensity) {
			this->intensity = intensity;
		}

		float getIntensity() {
			return intensity;
		}

		void setConeAngle(float coneAngle) {
			this->coneAngle = coneAngle;
		}

		float getConeAngle() {
			return coneAngle;
		}

		void setFocus(float focus) {
			this->focus = focus;
		}

		float getFocus() {
			return focus;
		}

		void setColor(int color) {
			this->color = color;
		}

		int getColor() {
			return color;
		}

        /**
         * Set the centre of the light in the X direction as a proportion of the image size.
         * @param centreX the center
         * @see #getCentreX
         */
		void setCentreX(float x) {
			centreX = x;
		}
		
        /**
         * Get the centre of the light in the X direction as a proportion of the image size.
         * @return the center
         * @see #setCentreX
         */
		float getCentreX() {
			return centreX;
		}

        /**
         * Set the centre of the light in the Y direction as a proportion of the image size.
         * @param centreY the center
         * @see #getCentreY
         */
		void setCentreY(float y) {
			centreY = y;
		}
		
        /**
         * Get the centre of the light in the Y direction as a proportion of the image size.
         * @return the center
         * @see #setCentreY
         */
		float getCentreY() {
			return centreY;
		}

        /**
         * Prepare the light for rendering.
         * @param width the output image width
         * @param height the output image height
         */
		void prepare(int width, int height) 
      {
			float lx = (float)(qCos(azimuth) * qCos(elevation));
			float ly = (float)(qSin(azimuth) * qCos(elevation));
			float lz = (float)qSin(elevation);
			direction = Vector3f(lx, ly, lz);
			direction.normalize();
			if (type != eDISTANT) {
				lx *= distance;
				ly *= distance;
				lz *= distance;
				lx += width * centreX;
				ly += height * centreY;
			}
			position = Vector3f(lx, ly, lz);
			realColor.set( color/*QColor(color) */);
			realColor.scale(intensity);
			cosConeAngle = (float)qCos(coneAngle);
		}
		
		//public Object clone() {
		//	try {
		//		Light copy = (Light)super.clone();
		//		return copy;
		//	}
		//	catch (CloneNotSupportedException e) {
		//		return NULL;
		//	}
		//}

		//public String toString() {
		//	return "Light";
		//}

	};

	class AmbientLight : public Light 
   {
		//public String toString() {
		//	return "Ambient Light";
		//}
	};

   class PointLight : public Light 
   {
		public:
       PointLight() {
			type = ePOINT;
		}

		//public String toString() {
		//	return "Point Light";
		//}
	};

	class DistantLight : public Light {
		public:
       DistantLight() {
			type = eDISTANT;
		}

		//public String toString() {
		//	return "Distant Light";
		//}
	};

	class SpotLight : public Light {
		public:
       SpotLight() {
			type = eSPOT;
		}

		//public String toString() {
		//	return "Spotlight";
		//}
	};


/**
 * A filter which produces lighting and embossing effects.
 */
class LightFilter : public WholeImageFilter 
{
    public:
    /**
     * Take the output colors from the input image.
     */
	static const int COLORS_FROM_IMAGE = 0;

    /**
     * Use constant material color.
     */
	static const int COLORS_CONSTANT = 1;

    /**
     * Use the input image brightness as the bump map.
     */
	static const int BUMPS_FROM_IMAGE = 0;

    /**
     * Use the input image alpha as the bump map.
     */
	static const int BUMPS_FROM_IMAGE_ALPHA = 1;

    /**
     * Use a separate image alpha channel as the bump map.
     */
	static const int BUMPS_FROM_MAP = 2;

    /**
     * Use a custom function as the bump map.
     */
	static const int BUMPS_FROM_BEVEL = 3;

   private:
	float bumpHeight;
	float bumpSoftness;
	int bumpShape;
	float viewDistance;
	Material material;
	QVector<Light *> lights;
	int colorSource;
	int bumpSource;
	Function2D *bumpFunction;
	QImage *environmentMap;
	int *envPixels;
	int envWidth, envHeight;

	// Temporary variables used to avoid per-pixel memory allocation while filtering
	Vector3f l;
	Vector3f v;
	Vector3f n;
	//Color4f shadedColor;
	//Color4f diffuse_color;
	//Color4f specular_color;
	Vector3f tmpv, tmpv2; 
 //  QVector3D l;
	//QVector3D v;
	//QVector3D n;
	Color4f shadedColor;
	Color4f diffuse_color;
	Color4f specular_color;
	//QVector3D tmpv, tmpv2;

enum BumpShape { Normal, Outer, Inner, Pillow, Up, Down };
   class localFunction2D : public Function2D 
   {
      private:
      Function2D *original;
      int bumpShape;

      public:
      localFunction2D(Function2D *orgFunction, int shape)
      {
         original = orgFunction;
         bumpShape = shape;
      }
      ~localFunction2D()
      {
         delete original;
      }
      float evaluate(float x, float y) 
      {
	      float v = original->evaluate( x, y );
	      switch ( bumpShape ) 
         {
	         case Outer:
		      v *= ImageMath::smoothStep( 0.45f, 0.55f, v );
		      break;
	      case Inner:
		      v = v < 0.5f ? 0.5f : v;
		      break;
	      case Pillow:
		      v = ImageMath::triangle( v );
		      break;
	      case Up:
		      v = ImageMath::circleDown( v );
		      break;
	      case Down:
		      v = ImageMath::gain( v, 0.75f );
		      break;
         }
	      return v;
      }
   };
	
      friend class localFunction2D;

	public:
   LightFilter() 
   {

      viewDistance = 10000.0f;
      colorSource = COLORS_CONSTANT;//COLORS_FROM_IMAGE;//COLORS_CONSTANT;
      bumpSource = BUMPS_FROM_IMAGE;
      envWidth = 1;
      envHeight = 1;
      environmentMap = 0;

      r255 = 1.0f/255.0f;

		//lights = new Vector();
		addLight(new DistantLight());
		bumpHeight = 1.0f;
		bumpSoftness = 50.0f;
		bumpShape = 0;
		//material = new Material();
		//l = new QVector3D();
		//v = new QVector3D();
		//n = new QVector3D();
		//shadedColor = new Color4f();
		//diffuse_color = new Color4f();
		//specular_color = new Color4f();
		//tmpv = new QVector3D();
		//tmpv2 = new QVector3D();
      bumpFunction = NULL;
	}

   ~LightFilter()
   {
      qDeleteAll(lights);
      delete bumpFunction;
   }

	void setMaterial( Material material ) {
		this->material = material;
	}

	Material *getMaterial() {
		return &material;
	}

	void setBumpFunction(Function2D *bumpFunction) {
		this->bumpFunction = bumpFunction;
	}

	Function2D *getBumpFunction() {
		return bumpFunction;
	}

	void setBumpHeight(float bumpHeight) {
		this->bumpHeight = bumpHeight;
	}

	float getBumpHeight() {
		return bumpHeight;
	}

	void setBumpSoftness(float bumpSoftness) {
		this->bumpSoftness = bumpSoftness;
	}

	float getBumpSoftness() {
		return bumpSoftness;
	}

	void setBumpShape(int bumpShape) {
		this->bumpShape = bumpShape;
	}

	int getBumpShape() {
		return bumpShape;
	}

	void setViewDistance(float viewDistance) {
		this->viewDistance = viewDistance;
	}

	float getViewDistance() {
		return viewDistance;
	}

	void setEnvironmentMap(QImage *environmentMap) {
		this->environmentMap = environmentMap;
		if (environmentMap != 0) {
			envWidth = environmentMap->width();
			envHeight = environmentMap->height();
			envPixels = (int *)environmentMap->bits();//getRGB( environmentMap, 0, 0, envWidth, envHeight, 0 );
		} else {
			envWidth = envHeight = 1;
			envPixels = NULL;
		}
	}

	QImage *getEnvironmentMap() {
		return environmentMap;
	}

	void setColorSource(int colorSource) {
		this->colorSource = colorSource;
	}

	int getColorSource() {
		return colorSource;
	}

	void setBumpSource(int bumpSource) {
		this->bumpSource = bumpSource;
	}

	int getBumpSource() {
		return bumpSource;
	}

	void setDiffuseColor(int diffuseColor) {
		material.diffuseColor = diffuseColor;
	}

	int getDiffuseColor() {
		return material.diffuseColor;
	}

	void addLight(Light *light) {
		lights.append(light);
	}
	
	void removeLight(Light *light) {
		lights.removeOne(light);
	}
	
	QVector<Light*> getLights() {
		return lights;
	}
	

	protected:
   /*static const */float r255;/* = 1.0f/255.0f;*/

	void setFromRGB( Color4f &c, int argb ) {
		c.set( ((argb >> 16) & 0xff) * r255, ((argb >> 8) & 0xff) * r255, (argb & 0xff) * r255, ((argb >> 24) & 0xff) * r255 );
      //c.set(QColor(argb)); // ok???
	}
	
   int *filterPixels( int width, int height, const int *inPixels, QRect /*transformedSpace*/ ) 
   {
      int index = 0;
      int *outPixels = new int[width * height];
      memset(outPixels,0,width*height);
      float width45 = qFabs(6.0f * bumpHeight);
      bool invertBumps = bumpHeight < 0;
      Vector3f position(0.0f, 0.0f, 0.0f);
      Vector3f viewpoint((float)width / 2.0f, (float)height / 2.0f, viewDistance);
      Vector3f normal;// = new QVector3D();
      Color4f envColor;// = new Color4f();
      //Color4f diffuseColor( QColor(material.diffuseColor) );
      Color4f specularColor( QColor(material.specularColor) );
      Color4f diffuseColor;
      diffuseColor.set( material.diffuseColor );
      specularColor.set( material.specularColor );
      //Color4f specularColor( QColor(Qt::green) );
      Function2D *bump = bumpFunction;
      int *softPixels = 0;
      // Apply the bump softness
      if (bumpSource == BUMPS_FROM_IMAGE || bumpSource == BUMPS_FROM_IMAGE_ALPHA || bumpSource == BUMPS_FROM_MAP || bump == NULL) 
      {
         if ( bumpSoftness != 0 ) {
            int bumpWidth = width;
            int bumpHeight = height;
            const int *bumpPixels = inPixels;
            if ( bumpSource == BUMPS_FROM_MAP && dynamic_cast<ImageFunction2D*>(bumpFunction) ) {
               ImageFunction2D *if2d = dynamic_cast<ImageFunction2D*>(bumpFunction);//(ImageFunction2D*)bumpFunction;
               bumpWidth = if2d->getWidth();
               bumpHeight = if2d->getHeight();
               bumpPixels = if2d->getPixels();
            }
            int *tmpPixels = new int[bumpWidth * bumpHeight];
            memset(tmpPixels,0,bumpWidth * bumpHeight);
            softPixels = new int[bumpWidth * bumpHeight];
            memset(softPixels,0,bumpWidth * bumpHeight);
            Kernel kernel;
            GaussianFilter::makeKernel( &kernel, bumpSoftness );
            GaussianFilter::convolveAndTranspose( kernel, bumpPixels, tmpPixels, bumpWidth, bumpHeight, true, false, false, GaussianFilter::WRAP_EDGES );
            GaussianFilter::convolveAndTranspose( kernel, tmpPixels, softPixels, bumpHeight, bumpWidth, true, false, false, GaussianFilter::WRAP_EDGES );
            bump = new ImageFunction2D(softPixels, bumpWidth, bumpHeight, ImageFunction2D::CLAMP, bumpSource == BUMPS_FROM_IMAGE_ALPHA);
            delete [] tmpPixels;

            Function2D *bbump = bump;
            if ( bumpShape != 0 ) 
               bump = new localFunction2D(bbump,bumpShape);
         } else if ( bumpSource != BUMPS_FROM_MAP )
            bump = new ImageFunction2D(inPixels, width, height, ImageFunction2D::CLAMP, bumpSource == BUMPS_FROM_IMAGE_ALPHA);
      }

      float reflectivity = material.reflectivity;
      float areflectivity = (1-reflectivity);
      Vector3f v1;// = new QVector3D();
      Vector3f v2;// = new QVector3D();
      Vector3f n;// = new QVector3D();
      //Light *lightsArray = new Light[lights.size()];
      //int i = 0;
      //foreach(Light light, lights)
      //   lightsArray[i++] = &light;
      //lights.copyInto(lightsArray);
      for (int i = 0; i < lights.count(); i++)
         lights[i]->prepare(width, height);

      float *heightWindow[3];
      for( int i = 0; i < 3; i++)
         heightWindow[i] = new float[width];
      for (int x = 0; x < width; x++)
      {
         heightWindow[0][x] = 0.0;
         heightWindow[1][x] = width45*bump->evaluate(x, 0);
         heightWindow[2][x] = 0.0;
      }

      // Loop through each source pixel
      for (int y = 0; y < height; y++) {
         bool y0 = y > 0;
         bool y1 = y < height-1;
         position.y = y;
         for (int x = 0; x < width; x++)
            heightWindow[2][x] = width45*bump->evaluate(x, y+1);
         for (int x = 0; x < width; x++) {
            bool x0 = x > 0;
            bool x1 = x < width-1;

            // Calculate the normal at this point
            if (bumpSource != BUMPS_FROM_BEVEL) {
               // Complicated and slower method
               // Calculate four normals using the gradients in +/- X/Y directions
               int count = 0;
               normal.x = normal.y = normal.z = 0;
               float m0 = heightWindow[1][x];
               float m1 = x0 ? heightWindow[1][x-1]-m0 : 0;
               float m2 = y0 ? heightWindow[0][x]-m0 : 0;
               float m3 = x1 ? heightWindow[1][x+1]-m0 : 0;
               float m4 = y1 ? heightWindow[2][x]-m0 : 0;

               if (x0 && y1) {
                  v1.x = -1.0f; v1.y = 0.0f; v1.z = m1;
                  v2.x = 0.0f; v2.y = 1.0f; v2.z = m4;
                  n.cross(v1, v2);
                  n.normalize();
                  if (n.z < 0.0)
                     n.z = -n.z;
                  normal.add(n);
                  count++;
               }

               if (x0 && y0) {
                  v1.x = -1.0f; v1.y = 0.0f; v1.z = m1;
                  v2.x = 0.0f; v2.y = -1.0f; v2.z = m2;
                  n.cross(v1, v2);
                  n.normalize();
                  if (n.z < 0.0)
                     n.z = -n.z;
                  normal.add(n);
                  count++;
               }

               if (y0 && x1) {
                  v1.x = 0.0f; v1.y = -1.0f; v1.z = m2;
                  v2.x = 1.0f; v2.y = 0.0f; v2.z = m3;
                  n.cross(v1, v2);
                  n.normalize();
                  if (n.z < 0.0)
                     n.z = -n.z;
                  normal.add(n);
                  count++;
               }

               if (x1 && y1) {
                  v1.x = 1.0f; v1.y = 0.0f; v1.z = m3;
                  v2.x = 0.0f; v2.y = 1.0f; v2.z = m4;
                  n.cross(v1, v2);
                  n.normalize();
                  if (n.z < 0.0)
                     n.z = -n.z;
                  normal.add(n);
                  count++;
               }

               // Average the four normals
               normal.x /= count;
               normal.y /= count;
               normal.z /= count;
            }
            if (invertBumps) {
               normal.x = -normal.x;
               normal.y = -normal.y;
            }
            position.x = x;

            if (normal.z >= 0) {
               // Get the material colour at this point
               if (colorSource == COLORS_FROM_IMAGE)
                  setFromRGB(diffuseColor, inPixels[index]);
               else
                  setFromRGB(diffuseColor, material.diffuseColor);
               if (reflectivity != 0 && environmentMap != NULL) {
                  //FIXME-too much normalizing going on here
                  tmpv2.set(viewpoint);
                  tmpv2.sub(position);
                  tmpv2.normalize();
                  tmpv.set(normal);
                  tmpv.normalize();

                  // Reflect
                  tmpv.scale( 2.0f*tmpv.dot(tmpv2) );
                  tmpv.sub(v);

                  tmpv.normalize();
                  setFromRGB(envColor, getEnvironmentMap(tmpv, inPixels, width, height));//FIXME-interpolate()
                  diffuseColor.x = reflectivity*envColor.x + areflectivity*diffuseColor.x;
                  diffuseColor.y = reflectivity*envColor.y + areflectivity*diffuseColor.y;
                  diffuseColor.z = reflectivity*envColor.z + areflectivity*diffuseColor.z;
               }
               // Shade the pixel
               Color4f c = phongShade(position, viewpoint, normal, diffuseColor, specularColor, material, lights/*Array*/);
               int alpha = inPixels[index] & 0xff000000;
               int rgb = ((int)(c.x * 255) << 16) | ((int)(c.y * 255) << 8) | (int)(c.z * 255);
               outPixels[index++] = alpha | rgb;
            } else
               outPixels[index++] = 0;
         }
         float *t = heightWindow[0];
         heightWindow[0] = heightWindow[1];
         heightWindow[1] = heightWindow[2];
         heightWindow[2] = t;
      }
      for( int i = 0; i < 3; i++)
         delete heightWindow[i];
      delete [] softPixels;
      if( bump != bumpFunction )
         delete bump;
      return outPixels;
   }

	Color4f phongShade(const Vector3f &position, const Vector3f &viewpoint, const Vector3f &normal, const Color4f &diffuseColor, const Color4f &specularColor, const Material &material, QVector<Light *> &lightsArray) 
   {
		shadedColor.set(diffuseColor);
		shadedColor.scale(material.ambientIntensity);

		for (int i = 0; i < lightsArray.length(); i++) {
			const Light *light = lightsArray.at(i);
			n.set(normal);
			l.set(light->position);
			if (light->type != eDISTANT)
				l.sub(position);
			l.normalize();
			float nDotL = n.dot(l);
			if (nDotL >= 0.0) {
				float dDotL = 0;
				
				v.set(viewpoint);
				v.sub(position);
				v.normalize();

				// Spotlight
				if (light->type == eSPOT) {
					dDotL = light->direction.dot(l);
					if (dDotL < light->cosConeAngle)
						continue;
				}

				n.scale(2.0f * nDotL);
				n.sub(l);
				float rDotV = n.dot(v);

				float rv;
				if (rDotV < 0.0)
					rv = 0.0f;
				else
//					rv = (float)Math.pow(rDotV, material.highlight);
					rv = rDotV / (material.highlight - material.highlight*rDotV + rDotV);	// Fast approximation to pow

				// Spotlight
				if (light->type == eSPOT) {
					dDotL = light->cosConeAngle/dDotL;
					float e = dDotL;
					e *= e;
					e *= e;
					e *= e;
					e = (float)qPow(dDotL, light->focus*10)*(1 - e);
					rv *= e;
					nDotL *= e;
				}
				
				diffuse_color.set(diffuseColor);
				diffuse_color.scale(material.diffuseReflectivity);
				diffuse_color.x *= light->realColor.x * nDotL;
				diffuse_color.y *= light->realColor.y * nDotL;
				diffuse_color.z *= light->realColor.z * nDotL;
				specular_color.set(specularColor);
				specular_color.scale(material.specularReflectivity);
				specular_color.x *= light->realColor.x * rv;
				specular_color.y *= light->realColor.y * rv;
				specular_color.z *= light->realColor.z * rv;
				diffuse_color.add(specular_color);
				diffuse_color.clamp( 0, 1 );
				shadedColor.add(diffuse_color);
			}
		}
		shadedColor.clamp( 0, 1 );
      //qDebug() << "phongShade gives " << (int)shadedColor;
		return shadedColor;
	}

	private:
    int getEnvironmentMap(Vector3f normal, const int * /*inPixels*/, int /*width*/, int /*height*/) 
    {
		if (environmentMap != NULL) {
			float angle = (float)qAcos(-normal.y);

			float x, y;
			y = angle/ImageMath::PI;

			if (y == 0.0f || y == 1.0f)
				x = 0.0f;
			else {
				float f = normal.x/(float)qSin(angle);

				if (f > 1.0f)
					f = 1.0f;
				else if (f < -1.0f) 
					f = -1.0f;

				x = (float)qAcos(f)/ImageMath::PI;
			}
			// A bit of empirical scaling....
			x = ImageMath::clamp(x * envWidth, 0.0, float(envWidth)-1.0);
			y = ImageMath::clamp(y * envHeight, 0.0, float(envHeight)-1.0);
			int ix = (int)x;
			int iy = (int)y;

			float xWeight = x-ix;
			float yWeight = y-iy;
			int i = envWidth*iy + ix;
			int dx = ix == envWidth-1 ? 0 : 1;
			int dy = iy == envHeight-1 ? 0 : envWidth;
			return ImageMath::bilinearInterpolate( xWeight, yWeight, envPixels[i], envPixels[i+dx], envPixels[i+dy], envPixels[i+dx+dy] );
		}
		return 0;
	}
	
	//public String toString() {
	//	return "Stylize/Light Effects...";
	//}

};
#endif // __LightFilter__
