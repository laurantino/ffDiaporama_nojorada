#ifndef __Vector3f__
#define __Vector3f__
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

#include "Tuple3f.h"

/**
 * Vector math package, converted to look similar to javax.vecmath.
 */
class Vector3f : public Tuple3f 
{
	public:
    Vector3f() {
		new (this) Vector3f( 0, 0, 0 );
	}
	
	Vector3f( float x[3] ) {
		this->x = x[0];
		this->y = x[1];
		this->z = x[2];
	}

	Vector3f( float x, float y, float z ) {
		this->x = x;
		this->y = y;
		this->z = z;
	}

	Vector3f( const Vector3f &t ) {
		this->x = t.x;
		this->y = t.y;
		this->z = t.z;
	}

	Vector3f( const Tuple3f &t ) {
		this->x = t.x;
		this->y = t.y;
		this->z = t.z;
	}

	float angle( const Vector3f &v ) const {
		return (float)qAcos( dot(v) / (length()*v.length()) );
	}

	float dot( const Vector3f &v ) const {
		return v.x * x + v.y * y + v.z * z;
	}

	void cross( const Vector3f &v1, const Vector3f &v2 ) {
		x = v1.y * v2.z - v1.z * v2.y;
		y = v1.z * v2.x - v1.x * v2.z;
		z = v1.x * v2.y - v1.y * v2.x;
	}

	float length() const {
		return (float)qSqrt( x*x+y*y+z*z );
	}

	void normalize() {
		float d = 1.0f/(float)qSqrt( x*x+y*y+z*z );
		x *= d;
		y *= d;
		z *= d;
	}

};
#endif // Vector3f

