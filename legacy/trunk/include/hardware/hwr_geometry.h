// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: hwr_geometry.h 305 2006-02-11 15:49:52Z jussip $
//
// Copyright (C) 1998-2004 by DooM Legacy Team.
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
//
//
//-----------------------------------------------------------------------------

#ifndef hwr_geometry_h
#define hwr_geometry_h 1

#include "hwr_states.h"
#include <map>

/**
  \brief Geometry class
*/
class Geometry
{
public:
    enum GeometryAttributes
    {
        VERTEX_ARRAY = 0,
        TEXCOORD_ARRAY,
        TEXCOORD_ARRAY0 = TEXCOORD_ARRAY,
        TEXCOORD_ARRAY1,
        TEXCOORD_ARRAY2,
        TEXCOORD_ARRAY3,
        TEXCOORD_ARRAY4,
        TEXCOORD_ARRAY5,
        TEXCOORD_ARRAY6,
        TEXCOORD_ARRAY7,
        COLOR_ARRAY,
        NORMAL_ARRAY,
        NUM_GEOMETRY_ATTRIBUTES
    };

private:

    GLfloat *vertex_array;
    GLfloat *tex_coord_arrays[State::MAX_TEXTURE_UNITS];
    GLfloat *normal_array;
    GLuint *color_array;
    GLushort *indices;
    GLuint *primitive_length;
    GLuint *primitive_type;
    GLushort num_primitives;

    static GLfloat *last_vertex_array;
    static GLfloat *last_tex_coord_arrays[State::MAX_TEXTURE_UNITS];
    static GLfloat *last_normal_array;
    static GLuint *last_color_array;

    // kind of garbage collector (TODO: should be an hash_map instead of a map)
    static std::map<GLfloat *, int> float_refcount;   // keep a reference count for all GLfloat pointers
    static std::map<GLuint *, int> uint_refcount;     // keep a reference count for all GLuint pointers
    static std::map<GLushort *, int> ushort_refcount; // keep a reference count for all GLishort pointers

    void EnableArrays();

public:

    /// Create a Geometry.
    Geometry();
    /// Destroy a Geometry.
    ~Geometry();
    /// Set the length of each primitive of the Geometry.
    void SetPrimitiveLength(GLuint *length);
    /// Set the primitive type (GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN) of each Geometry's primitives.
    void SetPrimitiveType(GLuint *type);
    /// Set the number of primitive in the Geometry.
    void SetNumPrimitives(int num);
    /// Change one of the attributes (VERTEX_ARRAY, COLOR_ARRAY, NORMAL_ARRAY, TEXCOORD_ARRAY, TEXCOORD_ARRAYi) of the Geometry.
    void SetAttributes(GeometryAttributes attr, void *array);
    /// Set the indices array. If null, we will assume indices are 0, 1, 2, ...
    void SetIndices(GLushort *indices);
    /// Create a basic textured rectangle
    void CreateTexturedRectangle(bool overwrite = false, float x1 = 0.0f, float y1 = 0.0f, float x2 = 1.0f, float y2 = 1.0f, float z = 0.0f);
    /// Change a vertex in the arrays
    inline void SetTexturedVertex(int which, float x, float y, float z, float s, float t);
    /// Draw the Geometry
    void Draw();
    /// Disbale all arrays
    static void DisableArrays();

    // memory management (TODO: for now we delete once we see the refcount is zero,
    //                          we might delete all null refcount when not playing a map
    //                          or when too much memory is already allocated for geometry)

    /// Increment the reference count of the float buffer
    inline static void Ref(GLfloat *buffer);
    /// Increment the reference count of the unsigned int buffer
    inline static void Ref(GLuint *buffer);
    /// Increment the reference count of the unsigned short buffer
    inline static void Ref(GLushort *buffer);
    /// Free the memory allocated for the float buffer if it's not used anymore (refcount == 0)
    inline static void UnrefDelete(GLfloat *buffer);
    /// Free the memory allocated for unsigned int buffer if it's not used anymore (refcount == 0)
    inline static void UnrefDelete(GLuint *buffer);
    /// Free the memory allocated for the unsigned short buffer if it's not used anymore (refcount == 0)
    inline static void UnrefDelete(GLushort *buffer);
    /// Free the memory allocated for all non used buffers (refcount == 0), see TODO above
    static void FreeAllBuffers();
};

inline void Geometry::SetTexturedVertex(int which, float x, float y, float z, float s, float t)
{
    GLfloat *vertex = &vertex_array[which * 3];
    vertex[0] = x;
    vertex[1] = y;
    vertex[2] = z;
    GLfloat *tex_coords = &tex_coord_arrays[0][which * 2];
    tex_coords[0] = s;
    tex_coords[1] = t;
}

inline void Geometry::Ref(GLfloat *buffer)
{
    float_refcount[buffer]++;
}

inline void Geometry::Ref(GLuint *buffer)
{
    uint_refcount[buffer]++;
}

inline void Geometry::Ref(GLushort *buffer)
{
    ushort_refcount[buffer]++;
}

inline void Geometry::UnrefDelete(GLfloat *buffer)
{
    if (buffer && (--float_refcount[buffer] == 0))
    {
        delete [] buffer;
    }
}

inline void Geometry::UnrefDelete(GLuint *buffer)
{
    if (buffer && (--uint_refcount[buffer] == 0))
    {
        delete [] buffer;
    }
}

inline void Geometry::UnrefDelete(GLushort *buffer)
{
    if (buffer && (--ushort_refcount[buffer] == 0))
    {
        delete [] buffer;
    }
}

#endif
