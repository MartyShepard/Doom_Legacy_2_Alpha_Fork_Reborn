// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: g_think.h 518 2007-12-31 02:54:58Z smite-meister $
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
/// \brief Definitions of the TypeInfo and Thinker classes.

#ifndef g_think_h
#define g_think_h 1

#include <map>
#include <vector>

using namespace std;


//==========================================================
// The information here applies to the original C code.
// It is saved for learning purposes.
//==========================================================

// Thinkers are used to implement all dynamic properties of a Doom map,
// eg. monsters, moving doors etc.

// Thinkers are a sort of C++ -like class construct created in plain C.
// thinker_t can be thought of as an abstract parent class with one virtual method,
// "function" (the doubly-linked list is just some simple extra stuff).
// thinker_t has lots of child "classes" ("classes" that "inherit" it), such as
// mobj_t, ceiling_t, vldoor_t etc.
// Any struct with thinker_t as its first member can be considered a child class of thinker_t.

// The normal way to invoke a thinker descendant is to call its "function" (acp1) with one parameter,
// a pointer to the class itself. This is just like a C++ method call passing the implicit
// 'this' pointer to a method.

// mobj_t : one function pointer in thinker_t (always P_MobjThinker), another available through state,
//     called _by_ P_MobjThinker.
// What makes mobjs different from one another is their initial attributes and their state cycle ("AI").
// in short, in the case of player mobjs, mobj_t.state is used to implement
// weapon state control and simple sequences where no weapon is required (like dying).
// Otherwise it is used to implement the mobj AI.

// Geometry-related thinker_t children:
// vldoor_t: T_VerticalDoor
// ceiling_t: T_MoveCeiling
// floormove_t: T_MoveFloor
// elevator_t: T_MoveElevator
// plat_t: T_PlatRaise

// fireflicker_t: T_FireFlicker
// lightflash_t: T_LightFlash
// strobe_t: T_StrobeFlash
// glow_t: T_Glow
// lightlevel_t: T_LightFade

// scroll_t: T_Scroll
// friction_t: T_Friction
// pusher_t: T_Pusher

// note also P_BlasterMobjThinker



//==========================================================
// TypeInfo

/// \brief An RTTI helper class for Thinkers, used in serialization.
///
/// This class takes care of runtime type identification in Legacy.
/// When a Thinker class is implemented, the corresponding typeinfo object
/// should be (statically) constructed (which adds its code to the map etc.)

class TypeInfo
{
private:
  typedef class Thinker* (*thinker_factory_t)();
  static map<unsigned, TypeInfo*>& id_map(); ///< mapping from class ID numbers to TypeInfo instances

/*
 * TypeInfo-Klasse  
 * type_id            : Eindeutige ID (unsigned) für jede Klasse/Thinker-Typ  
 * name               : Klartext-Name (z. B. "Mobj", "Ceiling", "Door")  
 * factory            : Factory-Funktion, die ein neues Objekt erzeugt (new cls())  
 * parent             : Zeiger auf Parent-Klasse → echte Vererbungs-Kette  
 * Statische id_map() : map<unsigned, TypeInfo*> – schnelles Lookup per ID
*/
public:
  unsigned           type_id; ///< class/type ID number
  const char        *name;    ///< a plaintext name for the class/type
  thinker_factory_t  factory; ///< well, a factory for the class
  TypeInfo          *parent;  ///< parent class TypeInfo (or NULL)
  // could contain even more info about the class

  TypeInfo(const char *n, thinker_factory_t f, TypeInfo *par);
  static TypeInfo *Find(unsigned code); ///< Searches the ID map for 'code'
};


class LArchive;

/// Macro used in declaring Thinker classes (defines the TypeInfo/serialization related members).
#define DECLARE_CLASS(cls) \
protected: \
  static inline Thinker *Create() { return new (cls); } \
public: \
  static  TypeInfo _type; \
  virtual TypeInfo *Type() const { return &_type; } \
  cls(); \
  virtual int Marshal(LArchive &a);



/// This macro implements the stuff declared in DECLARE_CLASS.
#define IMPLEMENT_CLASS(c,par) \
TypeInfo c::_type(#c, c::Create, &par::_type);


//==========================================================
// Thinker

/// \brief Base class for most active game objects.
/// \ingroup g_central
/*!
  All dynamic map elements are Thinker descendants.
  The Map stores them in a doubly linked ring.
*/

class Thinker
{
  friend class Map;
  friend class GameType;

  DECLARE_CLASS(Thinker);

private:
  /// linked list pointers
  Thinker  *prev;
  Thinker  *next;

public:
  /// RTTI helper function. If the Thinker descendant in question descends from T, returns (T *)this, otherwise NULL.
  /// Usage: XXX *p = thing->Inherits<XXX>();
  template<typename T>
  inline T *Inherits()
  {
    TypeInfo *p = Type();
    while (p)
      {
	if (p == &T::_type)
	  return reinterpret_cast<T *>(this);
	p = p->parent;
      }
    return NULL;
  }


  /// the map where the Thinker is situated
  class Map *mp;

  virtual ~Thinker();

  /// serialization (save/load)
  static int Serialize(Thinker *p, LArchive &a);
  static Thinker *Unserialize(LArchive &a);

  /// what it actually does :)
  virtual void Think() {}
  virtual void ClientThink() {} ///< movement interpolation

  /// pointer management
  virtual void CheckPointers() {}

  /// safe iteration through the linked ring
  inline Thinker *Next() const { return next; };
  inline Thinker *Prev() const { return prev; };

  /// memory management
  void *operator new(size_t size);
  void  operator delete(void *mem);
};

#endif
