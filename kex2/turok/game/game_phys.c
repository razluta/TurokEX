// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2012 Samuel Villarreal
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
//
// DESCRIPTION: Collision detection
//
//-----------------------------------------------------------------------------

#include "common.h"
#include "mathlib.h"
#include "game.h"
#include "actor.h"
#include "level.h"

#define TRYMOVE_COUNT           4
#define ONESIDED_FLOOR_DISTMAX  1.024f
#define EPSILON_FLOOR           0.975f
#define EPSILON_MOVE            0.1f
#define STEPHEIGHT              24

static plane_t g_fakeplane;

typedef enum
{
    TRT_NOHIT       = 0,
    TRT_SLOPE       = 1,
    TRT_STEEPSLOPE  = 2,
    TRT_OUTEREDGE   = 3,
    TRT_ACTOR       = 4
} tracetype_e;

typedef struct
{
    vec3_t      start;
    vec3_t      end;
    plane_t     *pl;
    vec3_t      hit;
    float       frac;
    tracetype_e type;
} trace_t;

//
// G_FindClosestPlane
//

plane_t *G_FindClosestPlane(vec3_t coord)
{
    unsigned int i;
    float dist;
    float curdist;
    plane_t *plane;
    kbool ok;

    // VERY TEMP

    ok = false;
    curdist = 0;
    plane = NULL;

    for(i = 0; i < g_currentmap->numplanes; i++)
    {
        plane_t *p;

        p = &g_currentmap->planes[i];

        if(Plane_PointInRange(p, coord[0], coord[2]))
        {
            dist = coord[1] - Plane_GetDistance(p, coord);

            if(p->flags & CLF_ONESIDED && dist < -16)
            {
                continue;
            }

            if(dist < 0)
            {
                dist = -dist;
            }

            if(ok)
            {
                if(dist < curdist)
                {
                    curdist = dist;
                    plane = p;
                }
            }
            else
            {
                plane = p;
                curdist = dist;
                ok = true;
            }
        }
    }

    return plane;
}

//
// G_ClipVelocity
//

static void G_ClipVelocity(vec3_t out, vec3_t velocity, vec3_t normal, float fudge)
{
    float d;
    vec3_t n;

    d = Vec_Dot(velocity, normal) * fudge;
    Vec_Scale(n, normal, d);
    Vec_Sub(out, velocity, n);
    d = Vec_Unit3(out);

    if(d != 0)
    {
        Vec_Scale(out, out, Vec_Unit3(velocity) / d);
    }
}

//
// G_TracePlane
//

static kbool G_TracePlane(trace_t *trace, plane_t *pl)
{
    float d;
    float dstart;
    float dend;
    
    d       = Vec_Dot(pl->points[0], pl->normal);
    dstart  = Vec_Dot(trace->start, pl->normal) - d;
    dend    = Vec_Dot(trace->end, pl->normal) - d;

    if(pl->flags & CLF_ONESIDED && !Plane_IsAWall(pl))
    {
        if(trace->start[1] -
            Plane_GetDistance(pl, trace->start) < -ONESIDED_FLOOR_DISTMAX)
        {
            return false;
        }
    }
    else if(!Plane_IsAWall(pl))
    {
        if(pl->normal[1] >= EPSILON_FLOOR)
        {
            return false;
        }
    }

    if(dstart > 0 && dend >= dstart)
    {
        return false;
    }

    if(dend <= 0 && dend < dstart)
    {
        trace->type = Plane_IsAWall(pl) ? TRT_STEEPSLOPE : TRT_SLOPE;
        trace->frac = (dstart - 0.03125f) / (dstart - dend);
        trace->pl = pl;
        return true;
    }

    return false;
}

//
// G_TraceActor
//

static kbool G_TraceActor(trace_t *trace, actor_t *actor)
{
    vec3_t dir;
    float x;
    float z;
    float vd;
    float d;

    Vec_Sub(dir, trace->end, trace->start);
    vd = Vec_Unit3(dir);
    Vec_Normalize3(dir);

    x = actor->origin[0] - trace->start[0];
    z = actor->origin[2] - trace->start[2];
    d = dir[0] * x + dir[2] * z;

    if(d >= 0 && vd != 0)
    {
        float dx;
        float dz;
        float ld;
        float len;

        vd = 1.0f / vd;

        dx = vd * dir[0];
        dz = vd * dir[2];
        ld = dx * x + dz * z;

        x = x - ld * dx;
        z = z - ld * dz;

        len = actor->width * actor->width - (x * x + z * z);

        if(len > 0)
        {
            trace->frac = ld - (float)sqrt(len) * vd;
            trace->pl   = actor->plane;
            trace->type = TRT_ACTOR;
            return true;
        }
    }

    return false;
}

//
// G_PathTraverse
//

void G_PathTraverse(plane_t *plane, trace_t *trace)
{
    int i;
    plane_t *pl;
    vec3_t distance;

    if(plane == NULL)
    {
        return;
    }

    pl = NULL;
    Vec_Sub(distance, trace->end, trace->start);

    for(i = 0; i < 3; i++)
    {
        float x;
        float z;
        float dx;
        float dz;
        float d;
        vec3_t vp1;
        vec3_t vp2;

        Vec_Copy3(vp1, plane->points[i]);
        Vec_Copy3(vp2, plane->points[(i + 1) % 3]);

        x = vp1[0] - vp2[0];
        z = vp2[2] - vp1[2];

        d = z * distance[0] + x * distance[2];

        if(d < 0)
        {
            dx = vp1[0] - trace->end[0];
            dz = vp1[2] - trace->end[2];

            if((dz * x + dx * z) / d < 0)
            {
                pl = plane->link[i];

                if(pl == NULL)
                {
                    vec3_t up;
                    vec3_t vp3;
                    vec3_t vec;
                    vec3_t vn;

                    Vec_Set3(up, 0, 1, 0);
                    Vec_Set3(vp3, vp2[0], vp2[1] + 1, vp2[2]);
                    Vec_Sub(vec, vp1, vp2);
                    Vec_Cross(vn, vec, up);
                    Vec_Normalize3(vn);

                    Plane_SetTemp(&g_fakeplane, vp1, vp2, vp3);
                    Vec_Copy3(g_fakeplane.normal, vn);

                    trace->pl = &g_fakeplane;
                    if(!G_TracePlane(trace, trace->pl))
                    {
                        continue;
                    }

                    trace->type = TRT_OUTEREDGE;
                    return;
                }

                break;
            }
        }
    }

    if(pl)
    {
        if(Plane_IsAWall(pl))
        {
            if(G_TracePlane(trace, pl))
            {
                return;
            }
        }

        trace->type = TRT_NOHIT;
        
        G_PathTraverse(pl, trace);
    }
}

//
// G_Trace
//
//

trace_t G_Trace(vec3_t start, vec3_t end, plane_t *plane)
{
    trace_t trace;

    Vec_Copy3(trace.start, start);
    Vec_Copy3(trace.end, end);

    trace.pl    = plane ? plane : G_FindClosestPlane(trace.start);
    trace.frac  = 1;
    trace.type  = TRT_NOHIT;

    if(!G_TracePlane(&trace, trace.pl))
    {
        G_PathTraverse(plane, &trace);
    }

    Vec_Lerp3(trace.hit, trace.frac, start, end);

    return trace;
}

//
// G_TryMoveActor
//
// Trace against surrounding planes and slide
// against it if needed, clipping velocity
// along the way
//

void G_TryMoveActor(actor_t *actor)
{
    plane_t *prevplane;

    prevplane = actor->plane;
    actor->plane = G_FindClosestPlane(actor->origin);
    actor->hitplane = NULL;

    if(actor->plane != NULL)
    {
        trace_t trace;
        vec3_t start;
        vec3_t end;
        vec3_t vel;
        int trymoves;
        int i;

        Vec_Copy3(start, actor->origin);
        Vec_Copy3(vel, actor->velocity);

        trymoves = 0;

        for(i = 0; i < TRYMOVE_COUNT; i++)
        {
            float tmpy;

            Vec_Add(end, start, vel);
            trace = G_Trace(start, end, actor->plane);

            if(trace.type == TRT_NOHIT)
            {
                break;
            }

            if(trace.type == TRT_STEEPSLOPE)
            {
                float height = (float)(sqrt(
                    trace.pl->points[0][1] * trace.pl->points[0][1] +
                    trace.pl->points[1][1] * trace.pl->points[1][1] +
                    trace.pl->points[2][1] * trace.pl->points[2][1]));

                if(height <= STEPHEIGHT || actor->origin[1] > height)
                {
                    continue;
                }
            }

            actor->hitplane = trace.pl;
            Vec_Copy3(start, trace.hit);

            tmpy = vel[1];

            G_ClipVelocity(vel, vel, trace.pl->normal, 1.01f);

            if(trace.type == TRT_STEEPSLOPE && vel[1] > tmpy)
            {
                vel[1] = tmpy;
            }

            if(Vec_Dot(vel, actor->velocity) <= 0)
            {
                actor->velocity[0] = 0;
                actor->velocity[2] = 0;
                break;
            }

            Vec_Copy3(actor->velocity, vel);
            trymoves++;
        }
    }
    else if(prevplane)
    {
        // actor has somehow ended up in the void. snap
        // it back to the last good known position
        Vec_Copy3(actor->origin, actor->prevorigin);
        actor->plane = prevplane;

        // we might not know the last known plane normal
        // so just negate velocity instead
        actor->velocity[0] = -actor->velocity[0];
        actor->velocity[2] = -actor->velocity[2];
    }
}