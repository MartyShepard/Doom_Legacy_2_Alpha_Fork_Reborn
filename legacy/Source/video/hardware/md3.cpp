// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: md3.cpp 536 2009-06-29 06:46:13Z smite-meister $
//
// Copyright (C) 2002-2007 by Doom Legacy Team
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
/// \brief Quake III MD3 models.

#include <string.h>
#include <math.h>

#include <GL/gl.h>

#include "doomtype.h"
#include "g_actor.h"
#include "g_map.h"

#include "m_swap.h"
#include "r_data.h"

#include "tables.h"
#include "w_wad.h"
#include "z_zone.h"
#include "hardware/md3.h"



const double pi = 3.1415926536; // M_PI;

static float NormLookup[256][256][3];

void MD3_InitNormLookup()
{
  int i, j;
  float phi, theta;
  for (i=0; i<256; i++)
    for (j=0; j<256; j++)
      {
	theta = 2*i*pi/255.0;
	phi   = 2*j*pi/255.0;
	NormLookup[i][j][0] = cos(phi) * sin(theta);
	NormLookup[i][j][1] = sin(phi) * sin(theta);
	NormLookup[i][j][2] = cos(theta);
      }
}

//======================================================
// modelcache_t
//======================================================

modelcache_t models;

MD3_player *modelcache_t::Load(const char *p)
{
  // TODO maybe check the existence of the model first?

  MD3_player *t = new MD3_player(p);

  // TODO only default skin can now be used
  bool result = t->Load(p, "default"); // +tagtype
  if (!result)
    {
      delete t;
      return NULL;
    }

  return t;
}

//======================================================
// TGATexture class
//======================================================

TGATexture::TGATexture(const char *n, int l)
  : LumpTexture(n, l, 0, 0)
{
  // Loads 24 and 32bpp (alpha channel) TGA textures
  // TGA format is little-endian (LSB)
  struct TGAheader
  {
    byte IDlength;  // image ID field lenght
    byte colormaptype; // 0 = no color maps used, 1 = color map included
    byte imagetype; // 2 = uncompressed truecolor
    byte colormapspec[5];
    Uint16 xorigin;
    Uint16 yorigin;
    Uint16 width;
    Uint16 height;
    byte bpp;
    byte imagedescriptor;  // bit flags
    // image ID
    // color map data
    // image data
    // possibly some other fields
  };

  TGAheader h;

  // read header
  if (fc.ReadLumpHeader(lump, &h, sizeof(TGAheader)) != sizeof(TGAheader))
    {
      I_Error("Error reading header from TGA lump '%s'.\n", n);
      return;
    }

  // Only support uncompressed truecolor images with no color maps
  if (h.imagetype != 2 || h.colormaptype != 0 || h.bpp < 24)
    {
      I_Error("Unsupported TGA file type '%s'.\n", n);
      return;
    }

  imageoffset = sizeof(TGAheader) + h.IDlength + 0;

  // Get the width, height, and color depth
  width = SHORT(h.width);
  height = SHORT(h.height);
  bpp = h.bpp;

  int imagesize = width * height * (bpp / 8);
  if (fc.LumpLength(lump) < imageoffset + imagesize)
    {
      I_Error("Corrupted TGA lump '%s'\n", n);
      return;
    }
	
  InitializeTexture();
}


TGATexture::~TGATexture()
{
  if (data)
    Z_Free(data);
  
  pixels = NULL; // data is the primary pointer, not pixels
}


void TGATexture::GLGetData()
{
  if (!pixels)
    {
      data = static_cast<byte *>(fc.CacheLumpNum(lump, PU_TEXTURE));
      pixels = &data[imageoffset];

      // TGAs are stored BGR and not RGB
      // 32 bit TGA files have alpha channel and get loaded differently
      if (bpp == 24)
	gl_format = GL_BGR;
      else
	gl_format = GL_BGRA;
    }
}


byte *TGATexture::GetData()
{
  return NULL; // TODO
};


//======================================================
// MD3_t class
//======================================================

// constructor
MD3_t::MD3_t()
{
  data = NULL;
  meshes = NULL;
}

// destructor
MD3_t::~MD3_t()
{
  if (data)
    Z_Free(data);

  if (meshes)
    Z_Free(meshes);
}

// given a *.md3 file name, loads the model to memory
bool MD3_t::Load(const string& filename)
{
  int i;
  //cout << "Loading md3 file " << filename << endl;

  int lump = fc.FindNumForName(filename.c_str());
  if (lump < 0)
    {
      CONS_Printf("MD3 file '%s' not found.\n", filename.c_str());
      return false;
    }

  data = static_cast<byte*>(fc.CacheLumpNum(lump, PU_MODEL));

  // make a correct-endian copy of header (saves some pointer dereferences during rendering...)
  MD3_header *h = reinterpret_cast<MD3_header*>(data);
  memcpy(&header, h, 4+4+64); // first three fields
  header.version = LONG(header.version);

  if (header.magic[0] != 'I' || header.magic[1] != 'D' ||
      header.magic[2] != 'P' || header.magic[3] != '3'
      || header.version != 15)
    {
      return false;
    }

  header.flags     = LONG(h->flags);
  header.numFrames = LONG(h->numFrames);
  header.numTags   = LONG(h->numTags);
  header.numMeshes = LONG(h->numMeshes);
  header.numSkins  = LONG(h->numSkins);
  header.ofsFrames = LONG(h->ofsFrames);
  header.ofsTags   = LONG(h->ofsTags);
  header.ofsMeshes = LONG(h->ofsMeshes);
  header.ofsEOF    = LONG(h->ofsEOF);


  // read frames
  frames = reinterpret_cast<MD3_frame*>(data + header.ofsFrames);

  // read tags
  int nt = header.numTags;
  tags = reinterpret_cast<MD3_tag*>(data + header.ofsTags);
  //for (i=0; i<nt; i++) printf("tag '%s'\n", tags[i].name);

  // init links
  links.resize(nt, NULL);

  // read meshes
  int nm = header.numMeshes;
  meshes = (MD3_mesh *)Z_Malloc(nm * sizeof(MD3_mesh), PU_MODEL, NULL);
  byte *meshbase = data + header.ofsMeshes;
  for (i=0; i<nm; i++)
    {
      // go to a new mesh
      MD3_mesh_header *mh = reinterpret_cast<MD3_mesh_header*>(meshbase);
      memcpy(&meshes[i].header, mh, 4+64); // first two fields

      meshes[i].header.flags = LONG(mh->flags);
      meshes[i].header.numFrames    = LONG(mh->numFrames);
      meshes[i].header.numShaders   = LONG(mh->numShaders);
      meshes[i].header.numVertices  = LONG(mh->numVertices);
      meshes[i].header.numTriangles = LONG(mh->numTriangles);
      meshes[i].header.ofsTriangles = LONG(mh->ofsTriangles);
      meshes[i].header.ofsShaders   = LONG(mh->ofsShaders);
      meshes[i].header.ofsST        = LONG(mh->ofsST);
      meshes[i].header.ofsVertices  = LONG(mh->ofsVertices);
      meshes[i].header.ofsEND       = LONG(mh->ofsEND);

      // load the shaders (skins)
      // TODO how are these used?
      meshes[i].shaders = reinterpret_cast<MD3_mesh_shader*>(meshbase + meshes[i].header.ofsShaders);
      //for (int j=0; j<meshes[i].header.numShaders; j++) printf("mesh %d, shader '%s'.\n", j, meshes[i].shaders[j].name);

      // triangles
      meshes[i].triangles = reinterpret_cast<MD3_mesh_triangle*>(meshbase + meshes[i].header.ofsTriangles);

      // texture coordinates (count == numVertices)
      meshes[i].texcoords = reinterpret_cast<MD3_mesh_texcoord*>(meshbase + meshes[i].header.ofsST);

      // and vertices (count == numVertices * numFrames)
      meshes[i].vertices = reinterpret_cast<MD3_mesh_vertex*>(meshbase + meshes[i].header.ofsVertices);

      meshes[i].material = NULL;
      meshbase += meshes[i].header.ofsEND;
    }

  //header.numFrames--; // why?
  return true;
}



// reads the *.skin files and loads the mesh skin textures accordingly
bool MD3_t::LoadSkinFile(const string& path, const string& filename)
{
  char meshname[50]; // enough? 
  char texname[50]; 

  //cout << "Loading skin file " << path << filename << endl;
  int lump = fc.FindNumForName((path + filename).c_str());
  if (lump < 0)
    {
      CONS_Printf("MD3 skin file '%s' not found.\n", filename.c_str());
      return false;
    }

  // parse the lump
  char * const base = static_cast<char*>(fc.CacheLumpNum(lump, PU_STATIC));
  base[fc.LumpLength(lump) - 1] = 0; // HACK, add NUL char at the end, overwrites part of last tag_ declaration...
  char *r = base;

  while (sscanf(r, "%49[^,],%49[^\n\r]", meshname, texname) == 2)
    {
      r = strchr(r, '\n');
      if (!r)
	break; // probably hit NUL in sscanf

      r += 1; // jump to char after newline

      if (strlen(texname) > 1 && strlen(meshname) > 0 &&
	  strncmp(meshname, "tag_", 4) != 0) // tags dont have skins
	{
	  // we must strip the path away from texname and replace it with our own:
	  char *p = strrchr(texname, '/'); // find last slash
	  if (!p)
	    continue;

	  string tname = path + string(p+1);

	  // Find the right mesh item to assign the skin to
	  for (int i=0; i < header.numMeshes; i++)
	    {
	      // check it the two names are the same
	      if (strcasecmp(meshes[i].header.name, meshname) == 0)
		meshes[i].material = materials.Get(tname.c_str(), TEX_lod);
	    }
	}
    }

  Z_Free(base);
  return true;
}

// link another model to a tag.
void MD3_t::Link(const char *tagname, MD3_t *m)
{
  for (int i=0; i<header.numTags; i++)
    if (strcmp(tags[i].name, tagname) == 0)
      {
	links[i] = m;
	return;
      }
}


// Draw the model interpolated between frames curr and next.
// interp must be in the range [0, 1]
void MD3_t::DrawInterpolated(MD3_animstate *st)
{
  GLfloat v1[3], v2[3], v[3], n1[3], n2[3], norm[3];
  int theta1, phi1, theta2, phi2;
  float interp = st->interp;

  // draw each mesh
  MD3_mesh *mesh = meshes;
  for (int m=0; m<header.numMeshes; m++, mesh++)
    {
      if (!mesh->material)
	continue;

      // pointers to the vertex data of the frames
      MD3_mesh_vertex *cv = &mesh->vertices[st->frame * mesh->header.numVertices];
      MD3_mesh_vertex *nv = &mesh->vertices[st->nextframe * mesh->header.numVertices];

      int i, j, k, n = mesh->header.numTriangles;
      GLfloat s, t;

      mesh->material->GLUse();
      glBegin(GL_TRIANGLES);
      for (i=0; i<n; i++)
	{
	  // TODO display lists? for each frame?
	  for (j=0; j<3; j++)
	    {
	      // vertex index
	      int index = mesh->triangles[i].index[j];

	      theta1 = cv[index].normal[0];
	      phi1   = cv[index].normal[1];
	      theta2 = nv[index].normal[0];
	      phi2   = nv[index].normal[1];

	      // interpolation
	      for (k=0; k<3; k++)
		{
		  n1[k] = NormLookup[theta1][phi1][k];
		  n2[k] = NormLookup[theta2][phi2][k];
		  norm[k] = (1-interp)*n1[k] + interp*n2[k];

		  v1[k] = cv[index].coord[k] / 64.0;
		  v2[k] = nv[index].coord[k] / 64.0;
		  v[k] = (1-interp)*v1[k] + interp*v2[k];
		}

	      s = mesh->texcoords[index].s;
	      t = mesh->texcoords[index].t;
	      glTexCoord2f(s, t);

	      glNormal3fv(norm);
	      glVertex3fv(v);
	    }

	}
      glEnd();
    }
}

// draw this model and all the models that have been linked to it
MD3_animstate *MD3_t::DrawRecursive(MD3_animstate *st, float pitch)
{
  int i, j, k;
  MD3_t *mp;

  GLfloat m[16]; // 4x4 matrix
  vec3_t trans;
  float *t1, *t2;
  rotation_t rot;
  float (*r1)[3], (*r2)[3];

  DrawInterpolated(st);
  for (i=0; i<header.numTags; i++)
    {
      mp = links[i];
      if (mp != NULL)
	{
	  // TODO matrices are not stored in a good format
	  t1 = tags[st->frame * header.numTags + i].origin;
	  t2 = tags[st->nextframe * header.numTags + i].origin;

	  r1   = tags[st->frame * header.numTags + i].rot;
	  r2   = tags[st->nextframe * header.numTags + i].rot;

	  for (j = 0; j<3; j++)
	    for (k = 0; k<3; k++)
	      rot[j][k] = (1-st->interp)*r1[j][k] + st->interp*r2[j][k];

	  for (j = 0; j<3; j++)	    
	    trans[j] = (1-st->interp)*t1[j] + st->interp*t2[j];

	  m[0] = rot[0][0];
	  m[1] = rot[0][1];
	  m[2] = rot[0][2];
	  m[3] = 0;
	  m[4] = rot[1][0];
	  m[5] = rot[1][1];
	  m[6] = rot[1][2];
	  m[7] = 0;
	  m[8] = rot[2][0];
	  m[9] = rot[2][1];
	  m[10]= rot[2][2];
	  m[11]= 0;
	  m[12] = trans[0];
	  m[13] = trans[1];
	  m[14] = trans[2];
	  m[15] = 1;

	  glPushMatrix();
	  glMultMatrixf(m);
	  st++; // important! st must be an array of animstates big enough for the 
	  // entire linked submodel complex! Not a very easy system, but should work...
	  // Note the order in which the models have been linked to one another
	  glRotatef(-0.5*pitch, 0, 1, 0);
	  st = mp->DrawRecursive(st, pitch);
	  glPopMatrix();
	}
    }

  return st;
}

//===============================================================
// MD3_player class
//===============================================================

MD3_player::MD3_player(const char *n)
  : cacheitem_t(n)
{
}

MD3_player::~MD3_player() {}


// loads a player model to memory
// path should end in a slash
bool MD3_player::Load(const string& path, const string& skin)
{
  // TODO: load a model from PK3 archive!
  // TODO: switch skins online!

  if (!legs.Load(path + string("lower.md3")) && !legs.Load(path + string("lower.MD3")))
    return false;

  if (!torso.Load(path + string("upper.md3")) && !torso.Load(path + string("upper.MD3")))
    return false;

  if (!head.Load(path + string("head.md3")) && !head.Load(path + string("head.MD3")))
    return false;

  if (!legs.LoadSkinFile(path, string("lower_") + skin + string(".skin")))
    return false;

  if (!torso.LoadSkinFile(path, string("upper_") + skin + string(".skin")))
    return false;

  if (!head.LoadSkinFile(path, string("head_") + skin + string(".skin")))
    return false;

  if (!LoadAnim(path + string("animation.cfg")))
    return false;

  legs.Link("tag_torso", &torso);
  torso.Link("tag_head", &head);

  return true;
}

// load animation sequences, *.cfg
bool MD3_player::LoadAnim(const string& filename)
{
  char buf[100];
  int first, num, loop, fps;

  //cout << "Loading animations file " << filename << endl;
  int lump = fc.FindNumForName(filename.c_str());
  if (lump < 0)
    {
      CONS_Printf("MD3 animations file '%s' not found.\n", filename.c_str());
      return false;
    }

  // parse the lump
  char * const base = static_cast<char*>(fc.CacheLumpNum(lump, PU_STATIC));
  char *r = base;

  int i = 0;
  while (sscanf(r, "%99[^\n]\n", buf) != EOF)
    {
      r = strchr(r, '\n') + 1; // jump to char after newline

      // TODO these probably also mean something...
      //if (strstr(buf, "sex") != NULL) {}
      //else if (strstr(buf, "headoffset") != NULL) {}
      //else if (strstr(buf, "footsteps") != NULL) {// footstep sound}
      //else
      if (strlen(buf) > 0 && sscanf(buf, "%d %d %d %d", &first, &num, &loop, &fps) == 4)
	{
	  anim[i].firstframe = first;
	  anim[i].lastframe  = first + num - 1;
	  anim[i].loopframes = loop;
	  anim[i].fps = fps;
	  i++;
	}
    }
  Z_Free(base);

  // legs and torso are independent models
  int skip = anim[MD3_animstate::LEGS_WALKCR].firstframe - anim[MD3_animstate::TORSO_GESTURE].firstframe;
  for (i=MD3_animstate::LEGS_WALKCR; i<MD3_animstate::MAX_ANIMATIONS; i++)
    {
      anim[i].firstframe -= skip;
      anim[i].lastframe -= skip;
    }

  return true;
}


//==================================================
// modelpres_t class
//==================================================

modelpres_t::modelpres_t(const char *name, int col, const char *skin)
{
  // TODO skin is not used (mesh textures (skins) should be stored here, not with meshes)
  color = col;
  mdl = models.Get(name);

  st[0].seq = -1;
  st[0].nextframe = 0;
  st[1].seq = -1;
  st[1].nextframe = 0;
  st[2].seq = MD3_animstate::MD3_animseq_e(0);
  st[2].nextframe = 0;

  SetAnim(Idle);

  for (int i=0; i<3; i++)
    {
      st[i].frame = st[i].nextframe;
      st[i].interp = 0.0f;
    }

  flags = 0;
  lastupdate = 0;
}


modelpres_t::~modelpres_t()
{
  mdl->Release();
}


// set a new animation sequence for the player
void modelpres_t::SetAnim(animseq_e seq)
{
  animseq = seq;

  // map Legacy animation sequence to Q3 sequences
  int torso_seq = st[1].seq, leg_seq = st[0].seq;
  switch (seq)
    {
    case Idle:
    default:
      torso_seq = MD3_animstate::TORSO_STAND;
      leg_seq = MD3_animstate::LEGS_IDLE;
      break;

    case Run:
      leg_seq = MD3_animstate::LEGS_WALK;
      break;

    case Back:
      leg_seq = MD3_animstate::LEGS_BACK;
      break;

    case Pain:
      torso_seq = MD3_animstate::TORSO_GESTURE;
      break;

    case Melee:
      torso_seq = MD3_animstate::TORSO_ATTACK2;
      break;

    case Shoot:
      torso_seq = MD3_animstate::TORSO_ATTACK;
      break;

    case Death1:
      torso_seq = leg_seq = MD3_animstate::BOTH_DEATH1;
      break;

    case Death2:
      torso_seq = leg_seq = MD3_animstate::BOTH_DEATH2;
      break;

    case Death3:
      torso_seq = leg_seq = MD3_animstate::BOTH_DEATH3;
      break;

    case Raise:
      torso_seq = MD3_animstate::TORSO_STAND;
      leg_seq = MD3_animstate::LEGS_IDLECR;
      break;
    }

  if (torso_seq != st[1].seq)
    {
      // change torso sequence
      st[1].seq = torso_seq;
      st[1].nextframe = mdl->anim[torso_seq].firstframe;
    }

  if (leg_seq != st[0].seq)
    {
      // leg sequences
      st[0].seq = leg_seq;
      st[0].nextframe = mdl->anim[leg_seq].firstframe;
    }

  // original Q3 sequences
  /*
  if (q3seq <= MD3_animstate::BOTH_DEAD3)
    {
      // common sequences
      st[0].seq = st[1].seq = q3seq;
      st[0].nextframe = st[1].nextframe = mdl->anim[q3seq].firstframe;
    }
  else if (q3seq <= MD3_animstate::TORSO_STAND2)
    {
      // torso sequences
      st[1].seq = q3seq;
      st[1].nextframe = mdl->anim[q3seq].firstframe;
    }
  else if (q3seq <= MD3_animstate::LEGS_TURN)
    {
      // leg sequences
      st[0].seq = q3seq;
      st[0].nextframe = mdl->anim[q3seq].firstframe;
    }
  */
}


void MD3_animstate::Advance(MD3_anim *anim, float dt)
{
  const int next_looping[MAX_ANIMATIONS] =
    {
      BOTH_DEAD1,
      BOTH_DEAD1,
      BOTH_DEAD2,
      BOTH_DEAD2,
      BOTH_DEAD3,
      BOTH_DEAD3,

      TORSO_STAND,
      TORSO_STAND,
      TORSO_STAND,
      TORSO_STAND,
      TORSO_STAND,
      TORSO_STAND,
      TORSO_STAND2,

      LEGS_WALKCR,
      LEGS_WALK,
      LEGS_RUN,
      LEGS_BACK,
      LEGS_SWIM,
      LEGS_IDLE,
      LEGS_IDLE,
      LEGS_IDLE,
      LEGS_IDLE,
      LEGS_IDLE,
      LEGS_IDLECR,
      LEGS_TURN
    };

  // current animation
  MD3_anim *a = &anim[seq];

  // advance interpolation by elapsed time
  float in = interp + a->fps*dt;

  if (in > 10)
    in = 10; // TODO long lapses are skipped

  while (in >= 1.0)
    {
      // advance one frame
      in--;
      frame = nextframe;
      if (++nextframe > a->lastframe)
	{
	  // find the next looping sequence
	  if (a->loopframes <= 0)
	    {
	      seq = next_looping[seq];
	      a = &anim[seq];
	      nextframe = a->firstframe;
	      continue;
	    }
	  else
	    nextframe -= a->loopframes;
	}
    }

  interp = in; // remainder
}

bool modelpres_t::Update(int nowtic)
{
  float dt = (nowtic - lastupdate) / 35.0f; // seconds

  // For some reason head is never animated? Always frame 0 for head.
  for (int i=0; i<2; i++)
    {
      st[i].Advance(mdl->anim, dt);
    }

  lastupdate = nowtic;
  return true;
}


// update and draw the player model
bool modelpres_t::Draw(const Actor *p)
{
  Update(p->mp->maptic);

  glFrontFace(GL_CW); // MD3s use this winding
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  vec_t<fixed_t> pos = p->pos;

#define Q2D_scale 0.75

  // TODO what is the correct floorlevel offset? Theoretically there
  // should be a tag_floor, but haven't seen one yet in any model. 24 seems to work.
  glTranslatef(pos.x.Float(), pos.y.Float(), pos.z.Float() + Q2D_scale*(mdl->legs.frames[st->frame].origin[2]+24));
  glScalef(Q2D_scale, Q2D_scale, Q2D_scale);
  glRotatef(Degrees(p->yaw), 0, 0, 1);
  // TODO yaw for head and torso, directed leg animation
  mdl->legs.DrawRecursive(&st[0], DegreesSigned(p->pitch));

  glPopMatrix();
  glFrontFace(GL_CCW);
  return true;
}


// TODO netcode for MD3 models
void modelpres_t::Pack(TNL::BitStream *s)
{
}

void modelpres_t::Unpack(TNL::BitStream *s)
{
}


void modelpres_t::PackAnim(TNL::BitStream *s)
{
}


void modelpres_t::UnpackAnim(TNL::BitStream *s)
{
}
