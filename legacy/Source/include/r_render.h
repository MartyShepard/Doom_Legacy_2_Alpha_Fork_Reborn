// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_render.h 463 2007-05-21 23:32:04Z smite-meister $
//
// A renderer "class" to give rendering functions access to
// private Map class data (geometry!)
// Renderer is a friend class of Map
//
//
//-----------------------------------------------------------------------------

#ifndef r_render_h
#define r_render_h 1

#include "m_fixed.h"
#include "r_main.h"

class PlayerInfo;
class PlayerPawn;
class Actor;
struct vertex_t;
struct seg_t;
struct sector_t;
struct subsector_t;
struct node_t;
struct side_t;
struct line_t;

class fadetable_t;

struct poly_t;
struct drawseg_t;
struct ffloor_t;
struct vissprite_t;
struct polyvertex_t;
struct extrasubsector_t;
struct gr_vissprite_t;
struct pspdef_t;
struct visplane_t;

class Rend
{
  friend class HWRend;       // FIXME: this is temporary. Later, HWRend should probably inherit from Rend
  friend class HWBsp;        // FIXME: this is temporary.
  friend class Subsector;    // FIXME: this is temporary.
  friend class spritepres_t; // this is a HACK too, for software renderer
private:
  const class Map *m; // currently rendered Map

  // FIXME temporary this is the dirtiest HACK ever
  // temporarily we have here COPIES of, or pointers to certain Map data members (geometry).
  // these MUST be set every time before rendering begins

  int             numvertexes;
  vertex_t*       vertexes;

  seg_t*          segs;

  int             numsectors;
  sector_t       *sectors;

  int             numsubsectors;
  subsector_t*    subsectors;

  int             numnodes;
  node_t*         nodes;

  int             numlines;
  line_t*         lines;

  side_t*         sides;

  // viewpoint
  fixed_t viewx, viewy, viewz;
  float   fviewx, fviewy, fviewz; // same with floats

  angle_t viewangle, aimingangle; // yaw, pitch, roll?

  PlayerPawn *viewplayer; // may be NULL
  Actor      *viewactor;  // may be NULL

public:

  /// Currently used fadetable
  lighttable_t   *base_colormap;


  // setting the geometry data pointers
  void SetMap(Map *m);

  // software renderer

  // r_bsp.cpp
  void R_AddLine(seg_t *line);
  bool R_CheckBBox(struct bbox_t &bbox);
  void R_ClipSolidWallSegment(int first, int last);
  void R_ClipPassWallSegment(int first, int last);

  // r_main.cpp
  angle_t R_PointToAngle(fixed_t x, fixed_t y);
  fixed_t R_PointToDist(fixed_t x, fixed_t y);
  fixed_t R_ScaleFromGlobalAngle (angle_t visangle);
  void R_SetupFrame(PlayerInfo *player);

  // r_plane.cpp
  void R_ClearPlanes();
  visplane_t *R_FindPlane(fixed_t height, Material *pic, int lightlevel, fixed_t xoff, fixed_t yoff, fadetable_t* planecolormap, ffloor_t* ffloor, bool sky);
  void R_DrawPlanes();
  void R_DrawSinglePlane(visplane_t* pl, bool handlesource);

  // r_segs.cpp
  void R_DrawWallSplats();
  void R_RenderMaskedSegRange(drawseg_t *ds, int x1, int x2);
  void R_RenderSegLoop();
  void R_StoreWallRange(int start, int stop);

  // r_things.cpp
  void R_SplitSprite(vissprite_t* sprite, Actor* thing);
  void R_DrawPSprite(pspdef_t *psp);
  void R_DrawPlayerSprites();
  void R_CreateDrawNodes();
  void R_DrawMasked();

  sector_t *R_FakeFlat(sector_t *sec, sector_t *tempsec,
               int *floorlightlevel, int *ceilinglightlevel,
               bool back);
  void R_Prep3DFloors(sector_t*  sector);
  void R_Subsector(int num);
  void R_RenderBSPNode (int bspnum);

  void R_RenderPlayerView(PlayerInfo *player);

  void R_RenderThickSideRange(drawseg_t *ds, int x1, int x2, ffloor_t *ffloor);

  void R_AddWallSplat(line_t  *wallline,
              int      sectorside,
              char    *patchname,
              fixed_t  top,
              fixed_t  wallfrac,
              int      flags);

  void R_AddSprites(sector_t *sec, int lightlevel);
  //void R_ProjectSprite (Actor* thing);
  void R_DrawSprite (vissprite_t* spr);

  // hardware renderer
  void releaseLineChains();
  void generateStacklist(sector_t *thisSector);
  void freeStacklists();
  bool areToptexturesMissing(sector_t *thisSector);
  bool areBottomtexturesMissing(sector_t *thisSector);
  void HWR_CorrectSWTricks();
};

extern Rend R;

#endif
