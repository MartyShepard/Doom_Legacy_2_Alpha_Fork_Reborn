// Emacs style mode select   -*- C++ -*- 
//---------------------------------------------------------------------
//
// $Id: functors.h 535 2009-06-14 02:18:35Z smite-meister $
//
// Copyright (C) 2003 by DooM Legacy Team.
//
//---------------------------------------------------------------------

/// \file
/// \brief Auxiliary STL functors for various purposes

#ifndef functors_h
#define functors_h 1

#include <string.h>

/*
struct equal_pointer
{
  bool operator()(const void* p1, const void* p2) const
  { return p1 == p2; }
};


struct less_pointer
{
  bool operator()(const void* p1, const void* p2) const
  { return p1 < p2; }
};
*/

struct equal_cstring
{
  bool operator()(const char* s1, const char* s2) const
  { return strcmp(s1, s2) == 0; }
};


struct less_cstring
{
  bool operator()(const char* s1, const char* s2) const
  { return strcmp(s1, s2) < 0; }
};


struct hash_cstring
{
  size_t operator()(const char* s) const
  { 
    // 64-bit FNV-1a hash
    size_t res = static_cast<size_t>(14695981039346656037ULL); 
    for ( ; *s; ++s)
      {
	res ^= static_cast<size_t>(*s);
	res *= static_cast<size_t>(1099511628211ULL);
      }
  
    return res;
  }
};


/*
struct equal_cstring8
{
  bool operator()(const char* s1, const char* s2) const
  { return strncmp(s1, s2, 8) == 0; }
};

struct hash_cstring8
{
  size_t operator()(const char* s) const
  {
    // 64-bit FNV-1a hash
    size_t res = static_cast<size_t>(14695981039346656037ULL); 
    for (int i=0; s[i] && i < 8; ++i)
      {
	res ^= static_cast<size_t>(s[i]);
	res *= static_cast<size_t>(1099511628211ULL);
      }
  
    return res;
  }
};
*/

#endif
