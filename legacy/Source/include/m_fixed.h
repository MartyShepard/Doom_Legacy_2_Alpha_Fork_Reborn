// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_fixed.h 474 2007-06-12 01:41:05Z smite-meister $
//
// Copyright (C) 2002-2007 by DooM Legacy Team.
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
/// \brief Fixed point arithmetic.

#ifndef m_fixed_h
#define m_fixed_h 1

#include <stdlib.h>
#include <math.h>
#include "doomtype.h"

namespace TNL { class BitStream; };


/// \brief Class for 16.16 fixed point arithmetic
///
/// Binary operators are not member functions but friends, to allow the promotion
/// of the left operand using non-explicit constructors.
/// The templated functions and their specializations are used to avoid unnecessary
/// construction of temporary fixed_t objects.
/// TODO replace internal int() casts with round()'s?
class fixed_t
{
public:
  typedef Sint32 value_t;
  typedef Sint64 large_t;

private:
  /// only nonstatic data member, the fixed point value of the variable
  value_t val;

public:
  enum
  {
    IBITS = 16,           ///< bits in the integer part
    FBITS = 16,           ///< bits in the fractional part
    UNIT  = (1 << FBITS), ///< unity
    FMIN  = -(1 << (IBITS-1)),    ///< min representable integer
    FMAX  = (1 << (IBITS-1)) - 1, ///< max representable integer
    FMASK = (UNIT - 1)    ///< mask for the fractional part
  };

  /// Constructors.
  /// Explicit constructors are only used when the call exactly matches them, i.e.
  /// the compliler never adds an implicit cast for the input arguments.
  inline fixed_t() {}
  inline fixed_t(int a) { val = a << FBITS; }
  inline fixed_t(float f)
  {
    // No reason for this slowdown. We don't check int's either.
    //if (f < FMIN)
    //  val = FMIN;
    //else if (f > FMAX)
    //  val = FMAX;
    //else
      val = value_t(f * float(UNIT));
  }
  inline fixed_t(double f) { val = value_t(f * double(UNIT)); }

  // copy constructor (has to be light!)
  inline fixed_t(const fixed_t& f) { val = f.val; }

  /// assignment operators
  inline fixed_t& operator= (const fixed_t& a) { val  = a.val;  return *this; }
  inline fixed_t& operator+=(const fixed_t& a) { val += a.val;  return *this; }
  inline fixed_t& operator-=(const fixed_t& a) { val -= a.val;  return *this; }
  inline fixed_t& operator<<=(int a) { val <<= a;  return *this; }
  inline fixed_t& operator>>=(int a) { val >>= a;  return *this; }

  /// templates for multiplication and division assignments
  template<typename U>
  inline fixed_t& operator*=(const U& a) { val = value_t(val*a); return *this; };
  template<typename U>
  inline fixed_t& operator/=(const U& a) { val = value_t(val/a); return *this; };


  /// unary minus
  inline fixed_t operator-() const
  {
    fixed_t res;
    res.val = -val; 
    return res;
  }

  /// Addition and subtraction, allowed only between fixed_t objects
  inline friend fixed_t operator+(const fixed_t& a, const fixed_t& b)
  {
    fixed_t res;
    res.val = a.val + b.val;
    return res;
  }

  inline friend fixed_t operator-(const fixed_t& a, const fixed_t& b)
  {
    fixed_t res;
    res.val = a.val - b.val;
    return res;
  }

  /// Multiplication template.
  /// Allow a non-fixed_t object on the left for efficiency (specialized fixed_t-fixed_t version follows!)
  //template<typename U>
  //inline friend fixed_t operator*(const U& a, const fixed_t& b)
  //{
  //  fixed_t res;
  //  res.val = value_t(a * b.val);
  //  return res;
  //}


  inline friend fixed_t operator*(double a, const fixed_t& b)
  {
    fixed_t res;
    res.val = value_t(a * b.val);
    return res;
  }

  inline friend fixed_t operator*(int a, const fixed_t& b)
  {
    fixed_t res;
    res.val = value_t(a * b.val);
    return res;
  }

  inline friend fixed_t operator*(const fixed_t& a, const fixed_t& b)
  {
    fixed_t res;
    res.val = (fixed_t::large_t(a.val) * fixed_t::large_t(b.val)) >> fixed_t::FBITS;
    //res.val = int(double(a.val) * double(b.val) / 65536.0);
    return res;
  }

  /// Quite esoteric: modulus division for fixed-point numbers (used in automap)
  inline fixed_t operator%(const fixed_t& a)
  {
    fixed_t res;
    res.val = val % a.val;
    return res;
  }

  /// logical NOT
  inline bool operator!() const { return (val == 0); }

  /// bit shifts
  inline fixed_t operator<<(int a) const
  {
    fixed_t res;
    res.val = val << a;
    return res;
  }

  inline fixed_t operator>>(int a) const
  {
    fixed_t res;
    res.val = val >> a;
    return res;
  }

  /// Division template.
  /// Allow a non-fixed_t object on the right for efficiency (specialized fixed_t-fixed_t version follows!)
  template<typename U>
  friend fixed_t operator/(const fixed_t& a, const U& b)
  {
    fixed_t res;
    res.val = value_t(a.val / b);
    return res;
  }
	
	/*Marty*/
	friend fixed_t operator/(const fixed_t& a, const fixed_t& b) {
		fixed_t res;
		if (b.val == 0) {
			res.val = (a.val < 0) ? MININT : MAXINT;
		} else if ((abs(a.val) >> 14) >= abs(b.val)) {
			res.val = (a.val ^ b.val) < 0 ? MININT : MAXINT;
		} else {
			res.val = (large_t(a.val) << FBITS) / large_t(b.val);
		}
		return res;
	}
	/* Und dann Klappts auch mitn 'friend' und beide könne sich im Darkroom sehen...
	 * ... wat da passiert interresiertm ich hauptdsache, die C++ strnge Regel
	 * läuft...
	 */
			
  /// comparison operators
  inline friend bool operator==(const fixed_t& a, const fixed_t& b) { return a.val == b.val; }
  inline friend bool operator!=(const fixed_t& a, const fixed_t& b) { return a.val != b.val; }
  inline friend bool operator<(const fixed_t& a, const fixed_t& b) { return a.val < b.val; }
  inline friend bool operator>(const fixed_t& a, const fixed_t& b) { return a.val > b.val; }
  inline friend bool operator<=(const fixed_t& a, const fixed_t& b) { return a.val <= b.val; }
  inline friend bool operator>=(const fixed_t& a, const fixed_t& b) { return a.val >= b.val; }

  /// basic functions
  inline friend fixed_t abs(const fixed_t& a) { fixed_t res; res.val = abs(a.val); return res; }
  inline friend fixed_t sqrt(const fixed_t& a) { fixed_t res; res.val = int(sqrtf(a.val)*256.0); return res; }

  /// "conversion operators", must not be implicitly used
  inline float   Float() const { return float(val) / float(UNIT); }

  // NOTE: the rounding funcs assume that right-shifting negative values continues the sign, ie. (-1 >> n) == -1.
  // In other words we assume a two's complement representation for negative numbers...
  /// round towards zero
  inline value_t trunc() const { return (val >= 0) ? val >> FBITS : ((val-1) >> FBITS)+1; /* return val >> FBITS; */ }
  /// round towards -infinity
  inline value_t floor() const { return val >> FBITS; /* return (val >= 0) ? val >> FBITS : ((val+1) >> FBITS)-1; */ }
  /// round towards +infinity
  inline value_t ceil()  const { return ((val-1) >> FBITS)+1; /* return (val > 0) ? ((val-1) >> FBITS)+1 : val >> FBITS; */ }

  /// returns the fractional part of _nonnegative_ number
  inline fixed_t frac() const { fixed_t res; res.val = val & FMASK; return res; }

  /// conveniences, remove them if you can!
  inline large_t  value() const { return val; }
  inline fixed_t& setvalue(value_t v) { val = v; return *this; }


  /// OpenTNL packing method
  void Pack(TNL::BitStream *s);

  /// OpenTNL unpacking method
  void Unpack(TNL::BitStream *s);
};



/// specialization for assignment-multiplying two fixed_t objects
template<>
inline fixed_t& fixed_t::operator*=(const fixed_t& a)
{
  val = (large_t(val) * large_t(a.val)) >> FBITS;
  //val = int(double(val) * double(a.val) / 65536.0);
  return *this;
}

/// specialization for assignment-dividing two fixed_t objects
template<>
inline fixed_t& fixed_t::operator/=(const fixed_t& a)
{
  if ((abs(val) >> 14) >= abs(a.val))
    val = (val ^ a.val) < 0 ? MININT : MAXINT;
  else
    val = (large_t(val) << FBITS) / large_t(a.val);
  //val = int(double(val) / double(a.val) * 65536.0);
  return *this;
}


/// specialization for multiplying two fixed_t objects
/*
template<>
inline fixed_t operator*(const fixed_t& a, const fixed_t& b)
{
  fixed_t res;
  res.val = (fixed_t::large_t(a.val) * fixed_t::large_t(b.val)) >> fixed_t::FBITS;
  //res.val = int(double(a.val) * double(b.val) / 65536.0);
  return res;
}
*/

/// specialization for dividing two fixed_t objects
/// Marty ... nop... da macht die Strenge Regel nicht mit
/// habe ich nach oben in die klasse verschoben
/*
template<>
inline fixed_t operator/(const fixed_t& a, const fixed_t& b)
{
  fixed_t res;
  // TODO FIXME the actual code should check against divide-by-zero!
  if ((abs(a.val) >> 14) >= abs(b.val))
    res.val = (a.val ^ b.val) < 0 ? MININT : MAXINT;
  else
    res.val = (fixed_t::large_t(a.val) << fixed_t::FBITS) / fixed_t::large_t(b.val);
  //res.val = int(double(a.val) / double(b.val) * 65536.0);
  return res;
}
*/


/// smallest possible increment
extern fixed_t fixed_epsilon; // was a static member of fixed_t, but it clogged up gdb output...

#endif
