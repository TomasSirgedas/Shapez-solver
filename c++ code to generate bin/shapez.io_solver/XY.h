#pragma once

#include <cmath>

class XY
{
public:
   XY( int px, int py )	                  { set( px, py ); }
   XY()						                  { set(0, 0); }
   ~XY()						                  {}
   void set( int px, int py )	            { x = px; y = py; }

   int len2() const                       { return x*x + y*y; }
   int dist2( const XY& p ) const         { return (p.x-x)*(p.x-x) + (p.y-y)*(p.y-y); }
   XY transposed() const                  { return XY( y, x ); }

   XY operator-() const				         { return XY( -x, -y ); }
   XY operator-( const XY& p ) const	   { return XY( x - p.x, y - p.y ); }
   XY operator+( const XY& p ) const	   { return XY( x + p.x, y + p.y ); }
   XY operator%( const XY& p ) const	   { return XY( x % p.x, y % p.y ); }
   XY operator*( int iMul ) const		   { return XY(x * iMul, y * iMul); }    
   XY operator/( int iDiv ) const		   { return XY(x / iDiv, y / iDiv); }    
   const XY& operator-=( const XY& p )    { return *this = *this - p; }
   const XY& operator+=( const XY& p )    { return *this = *this + p; }
   const XY& operator*=( int iMul )	      { return *this = *this * iMul; }	
   const XY& operator/=( int iDiv )	      { return *this = *this / iDiv; }	

   int  operator^( const XY& p ) const    { return x*p.y - y*p.x; }
   int  operator*( const XY& p ) const    { return x*p.x + y*p.y; }

   bool operator==( const XY& p ) const   { return x == p.x && y == p.y; }
   bool operator!=( const XY& p ) const   { return !(*this == p); }
   bool operator<( const XY& p ) const	   { return (y != p.y) ? (y < p.y) : (x < p.x); }

   XY transformed( int q ) const;

   static XY dir( int x ) { static XY a[] = { XY(1,0), XY(0,1), XY(-1,0), XY(0,-1) }; return a[x]; }

public:
    int x, y;
};

class XYit
{
public:
   explicit XYit( const XY& _size ) { size = _size; p = XY( 0, 0 ); }
   explicit XYit( const XY& _size, const XY& start ) { size = _size; p = start; }
   operator XY() const                 { return p; }
   operator bool() const               { return p.y < size.y; }
   void operator=( const XY& rhs )     { p = rhs; }
   XY    next() const                  { return p.x+1 >= size.x ? XY( 0, p.y+1 ) : XY( p.x+1, p.y ); }
   XYit& operator++()                  { if ( ++p.x >= size.x ) { p.x = 0; ++p.y; } return *this; }
   XYit& operator++( int n )           { return ++*this; }
   
   bool operator==( const XY& q ) const{ return p == q; }
   bool operator!=( const XY& q ) const{ return p != q; }
   bool operator<( const XY& q ) const	{ return p < q; }

public:
   XY size;
   XY p;
};

class XYf
{
public:
	XYf() { x = y = 0; }
	XYf( const XY& p ) { x = p.x; y = p.y; }
	XYf( const double& xx, const double& yy ) { x = xx; y = yy; }

	XYf operator+( const XYf& p ) const { return XYf(x + p.x, y + p.y); }
	XYf operator-( const XYf& p ) const { return XYf(x - p.x, y - p.y); }
	XYf operator-() const { return XYf() - *this; }
	XYf operator*(double m) const { return XYf(x * m, y * m); }
   XYf operator/(double m) const { return XYf(x / m, y / m); }	

   XYf& operator+=( const XYf& p ) { return *this = *this + p; }	
   XYf& operator-=( const XYf& p ) { return *this = *this - p; }	

	double operator*( const XYf& p ) { return x * p.x + y * p.y; }	
	double operator^( const XYf& p ) { return x * p.y - y * p.x; }

   double len2() const { return x*x + y*y; }
	double len() const;
   double dist2( const XYf& p ) const { return (*this-p).len2(); }
   double dist( const XYf& p ) const { return ::sqrt( dist2( p ) ); }
   
   XYf norm() const { return *this / len(); }   
   XYf rot90() const { return XYf( y, -x ); }

   XYf rotated( double a ) const { return XYf( x*cos(a) - y*sin(a), x*sin(a) + y*cos(a) ); }
   
   double closeTo( const XYf& p ) const { return ::fabs(x-p.x) + ::fabs(y-p.y) < 1e-12; }
//	string str() { return "(" + x.str() + "," + y.str() + ")"; }	

public:
	double x, y;
};

//class XYZ
//{
//public:
//   XYZ( int px, int py, int pz )	         { set( px, py, pz ); }
//   XYZ()						                  { set( 0, 0, 0 ); }
//   ~XYZ()						               {}
//   void set( int px, int py, int pz )	   { x = px; y = py; z = pz; }
//
//   int len2() const                       { return x*x + y*y + z*z; }
//   int dist2( const XYZ& p ) const        { return (p.x-x)*(p.x-x) + (p.y-y)*(p.y-y) + (p.z-z)*(p.z-z); }
//
//   XYZ operator-() const				      { return XYZ( -x, -y, -z ); }
//   XYZ operator-( const XYZ& p ) const	   { return XYZ( x - p.x, y - p.y, z - p.z ); }
//   XYZ operator+( const XYZ& p ) const	   { return XYZ( x + p.x, y + p.y, z + p.z ); }
//   XYZ operator*( int iMul ) const		   { return XYZ(x * iMul, y * iMul, z * iMul); }    
//   XYZ operator/( int iDiv ) const		   { return XYZ(x / iDiv, y / iDiv, z / iDiv); }    
//   const XYZ& operator-=( const XYZ& p )  { return *this = *this - p; }
//   const XYZ& operator+=( const XYZ& p )  { return *this = *this + p; }
//   const XYZ& operator*=( int iMul )	   { return *this = *this * iMul; }	
//   const XYZ& operator/=( int iDiv )	   { return *this = *this / iDiv; }	
//
//   int  operator*( const XYZ& p ) const   { return x*p.x + y*p.y + z*p.z; }
//
//   bool operator==( const XYZ& p ) const  { return x == p.x && y == p.y && z == p.z; }
//   bool operator!=( const XYZ& p ) const  { return !(*this == p); }
//   bool operator<( const XYZ& p ) const	{ return (z != p.z) ? (z < p.z) : (y != p.y) ? (y < p.y) : (x < p.x); }
//        
//public:
//    int x, y, z;
//};


class XYZ
{
public:
   XYZ( double px, double py, double pz ) { set( px, py, pz ); }
   XYZ() { set( 0, 0, 0 ); }
   ~XYZ() {}
   void set( double px, double py, double pz ) { x = px; y = py; z = pz; }

   double len2() const { return x*x + y*y + z*z; }
   double len() const { return sqrt( len2() ); }
   double dist2( const XYZ& p ) const { return (*this - p).len2(); }
   double dist( const XYZ& p ) const { return (*this - p).len(); }

   XYZ normalized() const { return *this / len(); }

   XYZ operator-() const { return XYZ( -x, -y, -z ); }
   XYZ operator-( const XYZ& p ) const { return XYZ( x - p.x, y - p.y, z - p.z ); }
   XYZ operator+( const XYZ& p ) const { return XYZ( x + p.x, y + p.y, z + p.z ); }
   XYZ operator*( double iMul ) const { return XYZ( x * iMul, y * iMul, z * iMul ); }
   XYZ operator/( double iDiv ) const { return XYZ( x / iDiv, y / iDiv, z / iDiv ); }
   const XYZ& operator-=( const XYZ& p ) { return *this = *this - p; }
   const XYZ& operator+=( const XYZ& p ) { return *this = *this + p; }
   const XYZ& operator*=( double iMul ) { return *this = *this * iMul; }
   const XYZ& operator/=( double iDiv ) { return *this = *this / iDiv; }

   XYZ operator^( const XYZ& p ) { return XYZ( y*p.z-z*p.y, z*p.x-x*p.z, x*p.y-y*p.x ); }

   double operator*( const XYZ& p ) const { return x*p.x + y*p.y + z*p.z; }

   bool operator==( const XYZ& p ) const { return x == p.x && y == p.y && z == p.z; }
   bool operator!=( const XYZ& p ) const { return !(*this == p); }
   bool operator<( const XYZ& p ) const { return (z != p.z) ? (z < p.z) : (y != p.y) ? (y < p.y) : (x < p.x); }

   XYZ XRotated( double a ) const {
      double cs = cos( a );
      double sn = sin( a );
      return XYZ( x, y * cs + z * sn, z * cs - y * sn );
   }
   XYZ YRotated( double a ) const {
      double cs = cos( a );
      double sn = sin( a );
      return XYZ( x * cs + z * sn, y, z * cs - x * sn );
   }
   XYZ ZRotated( double a ) const {
      double cs = cos( a );
      double sn = sin( a );
      return XYZ( x * cs + y * sn, y * cs - x * sn, z );
   }


public:
   double x, y, z;
};



class XYZW
{
public:
   XYZW( double x, double y, double z, double w ) : x(x),y(y),z(z),w(w) {}
   XYZW( const XYZ& p ) : x(p.x),y(p.y),z(p.z),w(1.) {}
   XYZW() : x(0), y(0), z(0), w(0) {}
   ~XYZW() {}

   XYZW operator-() const { return XYZW( -x, -y, -z, -w ); }
   XYZW operator-( const XYZW& p ) const { return XYZW( x - p.x, y - p.y, z - p.z, w - p.w ); }
   XYZW operator+( const XYZW& p ) const { return XYZW( x + p.x, y + p.y, z + p.z, w + p.w ); }
   XYZW operator*( double iMul ) const { return XYZW( x * iMul, y * iMul, z * iMul, w * iMul ); }
   XYZW operator/( double iDiv ) const { return XYZW( x / iDiv, y / iDiv, z / iDiv, w / iDiv ); }
   const XYZW& operator-=( const XYZW& p ) { return *this = *this - p; }
   const XYZW& operator+=( const XYZW& p ) { return *this = *this + p; }
   const XYZW& operator*=( double iMul ) { return *this = *this * iMul; }
   const XYZW& operator/=( double iDiv ) { return *this = *this / iDiv; }

   double operator*( const XYZW& p ) const { return x*p.x + y*p.y + z*p.z + w*p.w; }

   bool operator==( const XYZW& p ) const { return x == p.x && y == p.y && z == p.z && w == p.w; }
   bool operator!=( const XYZW& p ) const { return !(*this == p); }
   //bool operator<( const XYZW& p ) const { return (z != p.z) ? (z < p.z) : (y != p.y) ? (y < p.y) : (x < p.x); }

   //QPointF toPointF() const { return QPointF( x, y ); }
   XYZ toXYZ() const { return XYZ( x, y, z ) / w; }
   //QVector3D toQVector3D() const { return toXYZ().toQVector3D(); }

   bool eq( const XYZW& rhs, double tolerance = 1e-8 ) const;

public:
   double x, y, z, w;
};

class Matrix4x4
{
public:
   Matrix4x4() { m[0] = XYZW( 1, 0, 0, 0 ); m[1] = XYZW( 0, 1, 0, 0 ); m[2] = XYZW( 0, 0, 1, 0 ); m[3] = XYZW( 0, 0, 0, 1 ); }
   Matrix4x4( const XYZW& m0, const XYZW& m1, const XYZW& m2, const XYZW& m3 ) { m[0] = m0; m[1] = m1; m[2] = m2; m[3] = m3; }
   Matrix4x4( double m00, double m01, double m02, double m03, 
              double m10, double m11, double m12, double m13, 
              double m20, double m21, double m22, double m23, 
              double m30, double m31, double m32, double m33 ) { m[0] = XYZW(m00,m10,m20,m30); m[1] = XYZW(m01,m11,m21,m31); m[2] = XYZW(m02,m12,m22,m32); m[3] = XYZW(m03,m13,m23,m33); }
   XYZW& operator[]( int idx ) { return m[idx]; }
   XYZW operator[]( int idx ) const { return m[idx]; }
   XYZW operator*( const XYZW& v ) const { return m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3] * v.w; }
   //XYZ operator*( const XYZ& v ) const { return (*this * XYZW( v )).toXYZ(); }
   //QVector3D operator*( const QVector3D& p ) const { return (*this * XYZ(p.x(),p.y(),p.z())).toQVector3D(); }
   Matrix4x4 operator*( const Matrix4x4& rhs ) const { return Matrix4x4( *this * rhs[0], *this * rhs[1], *this * rhs[2], *this * rhs[3] ); }
   static Matrix4x4 scale( const XYZ& s ) { return Matrix4x4( { s.x, 0, 0, 0 }, { 0, s.y, 0, 0 }, { 0, 0, s.z, 0 }, { 0, 0, 0, 1 } ); }
   static Matrix4x4 translation( const XYZ& d ) { return Matrix4x4( { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { d.x, d.y, d.z, 1 } ); }
   static Matrix4x4 rotationX( double a ) { return Matrix4x4( { 1, 0, 0, 0 }, { 0, cos(a), sin(a), 0 }, { 0, -sin(a), cos(a), 0 }, { 0, 0, 0, 1 } ); }
   static Matrix4x4 rotationY( double a ) { return Matrix4x4( { cos(a), 0, sin(a), 0 }, { 0, 1, 0, 0 }, { -sin(a), 0, cos(a), 0 }, { 0, 0, 0, 1 } ); }
   static Matrix4x4 rotation( const XYZ& axis, double angle ) { XYZ u = axis.normalized(); double cs = cos(angle); double sn = sin(angle);    return Matrix4x4( { cs + u.x*u.x*(1-cs), u.y*u.x*(1-cs) + u.z*sn, u.z*u.x*(1-cs) - u.y*sn, 0 }, { u.x*u.y*(1-cs) - u.z*sn, cs + u.y*u.y*(1-cs), u.z*u.y*(1-cs) + u.x*sn, 0 }, { u.x*u.z*(1-cs) + u.y*sn, u.y*u.z*(1-cs) - u.x*sn, cs + u.z*u.z*(1-cs), 0 }, { 0, 0, 0, 1 } ); }

   void translate( double x, double y, double z ) { *this = *this * translation( XYZ(x,y,z) ); }
   double operator()( int r, int c ) const { return r == 0 ? m[c].x : r == 1 ? m[c].y : r == 2 ? m[c].z : m[c].w; }
   void rotateX( double angle ) { *this = *this * rotationX( angle ); }
   void rotateY( double angle ) { *this = *this * rotationY( angle ); }
   void scale( double s ) { *this = *this * scale( XYZ( s, s, s ) ); }
   void scale( double sx, double sy ) { *this = *this * scale( XYZ( sx, sy, 1 ) ); }

   Matrix4x4 inverted() const;

   Matrix4x4 pow( int p ) const;

   bool eq( const Matrix4x4& rhs, double tolerance = 1e-8 ) const;

public:
   XYZW m[4];
};
