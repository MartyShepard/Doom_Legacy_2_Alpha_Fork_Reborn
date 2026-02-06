// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: vect.h 367 2006-08-10 16:26:53Z smite-meister $
//
// Copyright (C) 2002-2006 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Algebraic three-vector template class.

#ifndef vect_h
#define vect_h 1

#include <math.h>
//#include<iostream>

namespace TNL { class BitStream; };

/// \brief Simple template class for three-vectors
///
/// Simple means that the class does not support the automatic promotion
/// of the left operand in binary operations, i.e. no templatized friend methods.
/// This makes the binary operations unsymmetric, the left operand dictates the return type.
///
/// Also, this class may not be computationally very efficient.
/// TODO consider replacing vec_t with an efficient three-vector template class, e.g.
/// TinyVector from the Blitz++ library (it should have similar semantics!):
///
/// template<typename T> typedef blitz::TinyVector<T,3> vec_t<T>;
/// or, with a minimal syntax change,
/// #define vec_t(T) blitz::TinyVector<T, 3>

template<typename T>
class vec_t
{
public:
  typedef vec_t<T> T_this; ///< type of *this
  T x, y, z; ///< the cartesian components of the vector

  /// constructors
  explicit inline vec_t()                 { x = y = z = 0; }
  inline vec_t(const vec_t &v)            { x = v.x; y = v.y; z = v.z; }
  explicit inline vec_t(const T& nx, const T& ny, const T& nz) { x = nx; y = ny; z = nz; }

  /// component extraction (also for assignment)
  //inline T& operator[](unsigned n) { return c[n]; }

  /// assignment
  template<typename U>
  inline T_this& Set(const U& nx, const U& ny, const U& nz) { x = nx; y = ny; z = nz; return *this; }
  template<typename U>
  inline T_this& operator= (const vec_t<U>& v) { x = v.x; y = v.y; z = v.z; return *this; }
  template<typename U>
  inline T_this& operator+=(const vec_t<U>& v) { x += v.x; y += v.y; z += v.z; return *this; }
  template<typename U>
  inline T_this& operator-=(const vec_t<U>& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
  template<typename U>
  inline T_this& operator*=(const U& a) { x *= a; y *= a; z *= a;  return *this; }
  template<typename U>
  inline T_this& operator/=(const U& a) { x /= a; y /= a; z /= a;  return *this; }
  inline T_this& operator<<=(int a) { x <<= a; y <<= a; z <<= a; return *this; }
  inline T_this& operator>>=(int a) { x >>= a; y >>= a; z >>= a; return *this; }

  /// unary minus
  inline T_this operator-() const { return T_this(-x, -y, -z); }

  /// utility methods
  inline T Norm()  const { return sqrt((x * x) + (y * y) + (z * z)); }
  inline T Norm2() const { return (x * x) + (y * y) + (z * z); }
  inline T XYNorm2() const { return (x * x) + (y * y); }
  template<typename U>
  inline T_this Project(const vec_t<U>& v) const
  {
    T temp = dot(v, *this) / v.Norm2();
    return T_this(v.x*temp, v.y*temp, v.z*temp);
  }

  /// polar coordinate methods
  T Theta() const { return atan2(sqrt((x * x) + (y * y)), z); }
  T Phi()   const { return atan2(y, x); }

  /// output method
  //void Print() const { std::cout << "(" << x << ',' <<  y << ',' << z << ")"; }

  /// Addition and subtraction
  template<typename U>
  inline T_this operator+(const vec_t<U>& v) const
  {
    //T_this res(*this);  res.x += v.x;  res.y += v.y;  res.z += v.z;  return res;
    return T_this(x + v.x, y + v.y, z + v.z);
  }

  template<typename U>
  inline T_this operator-(const vec_t<U>& v) const
  {
    //T_this res(*this);  res.x -= v.x;  res.y -= v.y;  res.z -= v.z;  return res;
    return T_this(x - v.x, y - v.y, z - v.z); 
  }

  /// scalar multiplication (NOTE: only from the right!)
  template<typename U>
  inline T_this operator*(const U& a) const
  {
    //T_this res(*this);  res.x *= a;  res.y *= a;  res.z *= a;  return res;
    //T_this res;  res.x = a*x;  res.y = a*y;  res.z = a*z;  return res;
    return T_this(a*x, a*y, a*z);
  }

  template<typename U>
  inline T_this operator/(const U& a) const
  {
    //T_this res(*this);  res.x /= a;  res.y /= a;  res.z /= a;  return res;
    return T_this(x/a, y/a, z/a);
  }

  /// bit shifts
  inline T_this operator<<(int a) const { return T_this(x << a, y << a, z << a); }
  inline T_this operator>>(int a) const { return T_this(x >> a, y >> a, z >> a); }

  /// comparisons
  template<typename U>
  inline bool operator==(const vec_t<U>& v) const { return (x == v.x && y == v.y && z == v.z); }
  template<typename U>
  inline bool operator!=(const vec_t<U>& v) const { return !(*this == v); }

  /// inner (dot) product
  template<typename U>
  inline friend U dot(const vec_t<T>& a, const vec_t<U>& b)
  {
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
  }

  /// cross (outer) product
  template<typename U>
  inline friend T_this cross(const vec_t<T>& a, const vec_t<U>& b)
  {
    return T_this(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
  }


  /// OpenTNL packing method
  inline void Pack(TNL::BitStream *s)
  {
    x.Pack(s);
    y.Pack(s);
    z.Pack(s);
  }

  /// OpenTNL unpacking method
  inline void Unpack(TNL::BitStream *s)
  {
    x.Unpack(s);
    y.Unpack(s);
    z.Unpack(s);
  }
};




/*
template<typename T>
vec_t<T> PolarVec(const T& radius, const T& theta, const T& phi)
{
  T rst = radius * sin(theta);
  return vec_t<T>(rst*cos(phi), rst*sin(phi), radius*cos(theta));
};
*/

#endif
