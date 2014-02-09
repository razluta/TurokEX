// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2013 Samuel Villarreal
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
//-----------------------------------------------------------------------------

#ifndef __WORLD_H__
#define __WORLD_H__

#include "common.h"
#include "array.h"
#include "linkedlist.h"
#include "camera.h"
#include "fx.h"
#include "collisionMap.h"

typedef enum {
    NODE_FRONT  = 0,
    NODE_BACK,
    NODE_SIDES
} nodeSide_t;

typedef struct worldNode_s {
    kexBBox                             bounds;
    kexPlane                            plane;
    kexArray<kexActor*>                 actors;
    bool                                bLeaf;
    struct worldNode_s                  *children[NODE_SIDES];
} worldNode_t;

#define MAX_AREA_DEPTH                  8
#define MAX_AREA_NODES                  512

typedef struct areaNode_s {
    kexBBox                             bounds;
    int                                 axis;
    float                               dist;
    kexLinklist<kexWorldObject>         objects;
    struct areaNode_s                   *children[NODE_SIDES];
} areaNode_t;

//-----------------------------------------------------------------------------
//
// kexWorld
//
//-----------------------------------------------------------------------------

class kexWorld {
public:
                                        kexWorld(void);
                                        ~kexWorld(void);

    void                                Init(void);
    void                                Tick(void);
    void                                LocalTick(void);
    bool                                Load(const char *mapFile);
    void                                Unload(void);
    const char                          *GetMapFileFromID(const int id);
    kexObject                           *ConstructObject(const char *className);
    kexActor                            *ConstructActor(const char *className);
    void                                AddActor(kexActor *actor);
    kexActor                            *SpawnActor(const char *className, const char *component,
                                            const kexVec3 &origin, const kexAngle &angles);
    kexActor                            *SpawnActor(kexStr &className, kexStr &component,
                                            kexVec3 &origin, kexAngle &angles);
    kexFx                               *SpawnFX(const char *name, kexGameObject *source, kexVec3 &velocity,
                                            kexVec3 &origin, kexQuat &rotation, kexFx *parentFx = NULL);
    void                                SpawnFX(const kexStr &str, kexGameObject *source, kexVec3 &velocity,
                                            kexVec3 &origin, kexQuat &rotation);
    void                                RemoveActor(kexActor *actor);
    void                                TeleportActor(kexActor *actor, const kexVec3 &position,
                                                      const kexAngle &angle, int sectorID = -1);
    void                                SpawnLocalPlayer(void);
    void                                Trace(traceInfo_t *trace);
    void                                StartSound(const char *name);
    void                                StartSound(const kexStr &name);
    void                                SetFogRGB(float r, float g, float b);
    void                                TriggerActor(const int targetID);

    bool                                IsLoaded(void) const { return bLoaded; }
    float                               DeltaTime(void) { return deltaTime; }
    kexCamera                           *Camera(void) { return &camera; };
    kexCollisionMap                     &CollisionMap(void) { return collisionMap; }
    kexVec3                             &GetGravity(void) { return gravity; }
    void                                SetGravity(const kexVec3 &in) { gravity = in; }
    const int                           GetTicks(void) const { return ticks; }
    float                               GetFogNear(void) { return currentFogNear; }
    void                                SetFogNear(float n) { fogNear = n; }
    float                               GetFogFar(void) { return currentFogFar; }
    void                                SetFogFar(float f) { fogFar = f; }
    float                               *GetCurrentFogRGB(void) { return currentFogRGB; }
    bool                                FogEnabled(void) { return bEnableFog; }
    void                                ToggleFog(bool toggle) { bEnableFog = toggle; }
    const int                           NumAreaNodes(void) const { return numAreaNodes; }
    bool                                &ReadyUnLoad(void) { return bReadyUnload; }
    const int                           MapID(void) const { return mapID; }
    int                                 &NextMapID(void) { return nextMapID; }

    kexLinklist<kexActor>               actors;
    kexLinklist<kexActor>               staticActors;
    kexLinklist<kexFx>                  fxList;

    kexActor                            *actorRover;
    kexFx                               *fxRover;
    worldNode_t                         worldNode;
    areaNode_t                          areaNodes[MAX_AREA_NODES];

    // TEMP
    kexVec4                             worldLightOrigin;
    kexVec4                             worldLightColor;
    kexVec4                             worldLightAmbience;
    kexVec4                             worldLightModelAmbience;
    //

    static kexHeapBlock                 hb_world;

    static void                         InitObject(void);

private:
    void                                BuildWorldNodes(void);
    void                                BuildAreaNodes(void);
    void                                AddWorldNode(worldNode_t *node, int split);
    areaNode_t                          *AddAreaNode(int depth, kexBBox &box);
    bool                                SetupChildWorldNode(worldNode_t *parent, worldNode_t *child,
                                            float *splitX, float *splitZ,
                                            nodeSide_t side, int split);
    void                                TraverseWorldNodes(worldNode_t *node, traceInfo_t *trace);

    bool                                bLoaded;
    bool                                bReadyUnload;
    int                                 mapID;
    int                                 nextMapID;
    kexStr                              title;
    kexCamera                           camera;
    kexCollisionMap                     collisionMap;
    kexVec3                             gravity;
    int                                 ticks;
    float                               time;
    float                               deltaTime;
    bool                                bEnableFog;
    float                               fogNear;
    float                               fogFar;
    float                               fogRGB[4];
    float                               currentFogNear;
    float                               currentFogFar;
    float                               currentFogRGB[4];
    int                                 validcount;
    int                                 numAreaNodes;
};

extern kexWorld localWorld;

#endif
