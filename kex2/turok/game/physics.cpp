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
// DESCRIPTION: Collision detection / Physics behavior
//
//-----------------------------------------------------------------------------

#include "common.h"
#include "mathlib.h"
#include "world.h"
#include "physics.h"

#define TRYMOVE_COUNT   5

enum {
    scPhysics_mass = 0,
    scPhysics_friction,
    scPhysics_airFriction,
    scPhysics_bounceDamp,
    scPhysics_stepHeight,
    scPhysics_rotorSpeed,
    scPhysics_rotorFriction,
    scPhysics_bRotor,
    scPhysics_bOrientOnSlope,
    scPhysics_rotorVector,
    scPhysics_end
};

static const sctokens_t physicsTokens[scPhysics_end+1] = {
    { scPhysics_mass,           "mass"                  },
    { scPhysics_friction,       "friction"              },
    { scPhysics_airFriction,    "airFriction"           },
    { scPhysics_bounceDamp,     "bounceDamp"            },
    { scPhysics_stepHeight,     "stepHeight"            },
    { scPhysics_rotorSpeed,     "rotorSpeed"            },
    { scPhysics_rotorFriction,  "rotorFriction"         },
    { scPhysics_bRotor,         "bRotor"                },
    { scPhysics_bOrientOnSlope, "bOrientOnSlope"        },
    { scPhysics_rotorVector,    "rotorVector"           },
    { -1,                       NULL                    }
};

DECLARE_CLASS(kexPhysics, kexObject)

//
// kexPhysics::kexPhysics
//

kexPhysics::kexPhysics(void) {
    this->mass                  = 1800;
    this->friction              = 1;
    this->airFriction           = 0;
    this->bounceDamp            = 0;
    this->stepHeight            = 48;
    this->rotorSpeed            = 0;
    this->rotorFriction         = 1;
    this->waterHeight           = 0;
    this->bRotor                = false;
    this->bOrientOnSlope        = false;
    this->bOnGround             = false;
    this->bInWater              = false;
    this->waterLevel            = WLT_INVALID;
    this->groundGeom            = NULL;
    this->groundMesh            = NULL;
    this->sector                = NULL;

    this->rotorVector.Clear();
    this->velocity.Clear();
}

//
// kexPhysics::~kexPhysics
//

kexPhysics::~kexPhysics(void) {
}

//
// kexPhysics::Parse
//

void kexPhysics::Parse(kexLexer *lexer) {
    // read into nested block
    lexer->ExpectNextToken(TK_LBRACK);
    lexer->Find();

    while(lexer->TokenType() != TK_RBRACK) {
        switch(lexer->GetIDForTokenList(physicsTokens, lexer->Token())) {
        case scPhysics_mass:
            this->mass = (float)lexer->GetFloat();
            break;
        case scPhysics_friction:
            this->friction = (float)lexer->GetFloat();
            break;
        case scPhysics_airFriction:
            this->airFriction = (float)lexer->GetFloat();
            break;
        case scPhysics_bounceDamp:
            this->bounceDamp = (float)lexer->GetFloat();
            break;
        case scPhysics_rotorSpeed:
            this->rotorSpeed = (float)lexer->GetFloat();
            break;
        case scPhysics_rotorFriction:
            this->rotorFriction = (float)lexer->GetFloat();
            break;
        case scPhysics_bRotor:
            this->bRotor = (lexer->GetNumber() > 0);
            break;
        case scPhysics_bOrientOnSlope:
            this->bRotor = (lexer->GetNumber() > 0);
            break;
        case scPhysics_rotorVector:
            this->rotorVector = lexer->GetVector3();
            break;
        default:
            if(lexer->TokenType() == TK_IDENIFIER) {
                parser.Error("kexPhysics::Parse: unknown token: %s\n",
                    lexer->Token());
            }
            break;
        }
        
        lexer->Find();
    }
}

//
// kexPhysics::GroundDistance
//

float kexPhysics::GroundDistance(void) {
    kexVec3 org = owner->GetOrigin();

    if(groundGeom == NULL) {
        return 0;
    }

    return (org[1] - groundGeom->GetDistance(org));
}

//
// kexPhysics::OnSteepSlope
//

bool kexPhysics::OnSteepSlope(void) {
    if(groundGeom == NULL) {
        return false;
    }
    return (groundGeom->plane.Normal().Dot(-localWorld.GetGravity()) <= ONPLANE_EPSILON);
}

//
// kexPhysics::OnGround
//

bool kexPhysics::OnGround(void) {
    if(groundGeom == NULL) {
        return false;
    }
    if(OnSteepSlope()) {
        return false;
    }
    return GroundDistance() <= ONPLANE_EPSILON;
}

//
// kexPhysics::ImpactVelocity
//

void kexPhysics::ImpactVelocity(kexVec3 &vel, kexVec3 &normal, const float force) {
    kexVec3 dir = vel;
    float d = vel.Unit();
    float bounce = force;

    if(bounceDamp != 0) {
        if(d >= 1.05f) {
            bounce = (force + bounceDamp);

            if(d < 16.8f && force < 1.0f)
                bounce = 0.2f;
        }
    }

    vel = dir - (normal * (dir.Dot(normal) * bounce));

    if(d != 0) {
        vel *= (dir.Unit() / d);
    }
}

//
// kexPhysics::ApplyFriction
//

void kexPhysics::ApplyFriction(void) {
    float speed;

    speed = velocity.Unit();

    if(speed < VELOCITY_EPSILON) {
        velocity.x = 0;
        velocity.z = 0;

        if(bInWater && waterLevel == WLT_UNDER) {
            velocity.y = 0;
        }
    }
    else {
        float clipspeed = speed - (speed * friction);

        if(clipspeed < 0) {
            clipspeed = 0;
        }

        clipspeed /= speed;

        // de-accelerate velocity
        velocity.x = velocity.x * clipspeed;
        velocity.z = velocity.z * clipspeed;

        if(bInWater && waterLevel == WLT_UNDER) {
            velocity.y = velocity.y * clipspeed;
        }
    }

    if(!bInWater) {
        float yFriction = 0;

        if(airFriction == 0) {
            float dist;

            if(groundGeom == NULL) {
                return;
            }

            dist = GroundDistance();

            // apply vertical friction only if we're rubbing up against the floor
            if((dist > ONPLANE_EPSILON || dist < -ONPLANE_EPSILON) ||
                velocity.Dot(groundGeom->plane.Normal()) <= 0) {
                return;
            }

            yFriction = friction;
        }
        else {
            yFriction = airFriction;
        }

        speed = velocity.y;

        if(speed < VELOCITY_EPSILON) {
            velocity.y = 0;
        }
        else {
            float clipspeed = speed - (speed * yFriction);

            if(clipspeed < 0) {
                clipspeed = 0;
            }

            clipspeed /= speed;

            // de-accelerate velocity
            velocity.y = velocity.y * clipspeed;
        }
    }
}

//
// kexPhysics::ClimbOnSurface
//

void kexPhysics::ClimbOnSurface(kexVec3 &start, const kexVec3 &end, kexTri *tri) {
    kexVec3 dir;
    float dist;
    float lenxz;
    float leny;
    float y1;
    float y2;

    if((end - *tri->point[0]).Dot(tri->plane.Normal()) > 0) {
        return;
    }

    y1 = tri->GetDistance(start);
    y2 = tri->GetDistance(end);

    dir.Set(end[0] - start[0], y2 - y1, end[2] - start[2]);
    lenxz = dir.ToVec2().UnitSq();
    leny = dir[1]*dir[1]+lenxz;

    if(leny == 0) {
        start[1] = y1;
        return;
    }

    dist = kexMath::Sqrt(lenxz / leny);

    start[0] = (end[0] - start[0]) * dist + start[0];
    start[1] = (y2 - y1) * dist + y1;
    start[2] = (end[2] - start[2]) * dist + start[2];
}

//
// kexPhysics::CheckWater
//

void kexPhysics::CheckWater(float height) {
    if(owner && sector && sector->area) {
        waterLevel = sector->area->GetWaterLevel(owner->GetOrigin(), height);
        waterHeight = sector->area->WaterPlane();
        bInWater = (waterLevel > WLT_OVER);
    }
}

//
// kexPhysics::GetWaterDepth
//

float kexPhysics::GetWaterDepth(void) {
    float dist;
    float sink;

    if(groundGeom == NULL) {
        return 0;
    }

    dist = GroundDistance();

    if(dist <= ONPLANE_EPSILON) {
        dist = 0;
    }

    sink = dist;

    if(dist * 0.125f >= 2) {
        dist = dist * 0.125f;
    }
    else {
        dist = 2;
    }

    return dist * 4;
}

//
// kexPhysics::Think
//

void kexPhysics::Think(const float timeDelta) {
    traceInfo_t trace;
    float       slope;
    int         moves;
    int         hits;
    float       d;
    float       time;
    float       currentMass;
    float       massAmount;
    float       radius;
    float       height;
    float       stepFraction;
    bool        bCanStep;
    kexVec3     start;
    kexVec3     end;
    kexVec3     direction;
    kexVec3     vel;
    kexVec3     normals[TRYMOVE_COUNT];
    kexVec3     slideNormal;
    kexVec3     gravity;
    kexVec3     cDir;

    if(owner == NULL || timeDelta == 0) {
        return;
    }
    if(owner->bStatic == true) {
        return;
    }

    // don't update on first two ticks
    if(localWorld.GetTicks() <= 1) {
        return;
    }

    currentMass = (bInWater && waterLevel >= WLT_BETWEEN) ? 0 : mass;
    start       = owner->GetOrigin();
    moves       = 0;
    time        = timeDelta;
    massAmount  = (currentMass * timeDelta);
    radius      = owner->Radius();
    height      = owner->BaseHeight();
    bCanStep    = true;
    gravity     = localWorld.GetGravity();

    trace.owner = owner;
    trace.bUseBBox = true;
    trace.localBBox.min.Set(-(radius * 0.5f), 0, -(radius * 0.5f));
    trace.localBBox.max.Set(radius * 0.5f, height, radius * 0.5f);
    trace.bbox = trace.localBBox;
    trace.bbox.min += start;
    trace.bbox.max += start;
    trace.sector = &sector;
    // resize box to account for movement
    trace.bbox |= (velocity * time);

    // handle interactions with touchable objects
    if(owner->bCanPickup && owner->areaNode) {
        for(kexWorldObject *obj = owner->areaNode->objects.Next();
            obj != NULL;
            obj = obj->areaLink.Next()) {
                if(obj == owner || !obj->bTouch) {
                    continue;
                }
                if(obj->Bounds().IntersectingBox(trace.bbox)) {
                    obj->OnTouch(owner);
                }
        }
    }

    trace.start = start;
    trace.end = start + (gravity * mass) * mass;
    trace.dir = gravity;

    // need to determine if we're standing on the ground or not
    localWorld.Trace(&trace);
    if(trace.hitTri) {
        groundGeom = trace.hitTri;
        groundMesh = trace.hitMesh;
    }

    if(trace.hitActor && trace.hitActor->bTouch) {
        trace.hitActor->OnTouch(owner);
    }

    if(bInWater && waterLevel >= WLT_BETWEEN) {
        bOnGround = false;

        // slowly drift to the bottom
        if(waterLevel == WLT_UNDER) {
            kexVec3 sinkVel = (-gravity * (GetWaterDepth() / timeDelta)) + velocity;
            velocity = velocity.Lerp(sinkVel, -0.2f * timeDelta);
        }
    }
    else {
        bOnGround = OnGround();

        // handle freefall if not touching the ground
        if(!bOnGround) {
            velocity += (gravity * massAmount);
        }
        else {
            vel = velocity;

            // project along the ground plane
            if(velocity.Dot(groundGeom->plane.Normal()) <= 1.024f) {
                float oldDist;
                float newDist;

                ImpactVelocity(velocity, groundGeom->plane.Normal(), 1.024f);

                if(vel.Dot(velocity) > 0) {
                    oldDist = vel.UnitSq();
                    if(oldDist > 1) {
                        newDist = velocity.UnitSq();
                        if(oldDist != newDist && newDist > 1) {
                            velocity *= kexMath::Sqrt(oldDist / newDist);
                        }
                    }
                }

                normals[moves++] = trace.hitNormal;
            }
        }
    }

    if(sector && sector->flags & CLF_CLIMB) {
        bCanStep = false;
    }

    // fudge the origin if we're slightly clipping below the floor
    trace.start = start - (gravity * (stepHeight * 0.5f));
    trace.end = start;
    localWorld.Trace(&trace);

    if(trace.fraction != 1 && !trace.hitActor) {
        start = trace.hitVector - (gravity * 1.024f);
        owner->SetOrigin(start);
        owner->LinkArea();
    }

    for(int i = 0; i < TRYMOVE_COUNT; i++) {
        start = owner->GetOrigin();
        end = start + (velocity * time);

        direction = (end - start);
        direction.Normalize();

        trace.start = start;
        trace.end = end;
        trace.dir = direction;

        // trace through world
        localWorld.Trace(&trace);
        time -= (time * trace.fraction);

        if(trace.fraction >= 1) {
            // went the entire distance
            owner->SetOrigin(end);
            owner->LinkArea();
            break;
        }

        // check if climbing up along a surface
        if(sector && sector->flags & CLF_CLIMB) {
            owner->SetOrigin(trace.hitVector);
            ClimbOnSurface(owner->GetOrigin(), end, &sector->lowerTri);

            // nudge origin away from plane
            owner->GetOrigin() -= (direction * 0.1f);
            owner->LinkArea();

            velocity.Clear();
            break;
        }
        else {
            // update origin and nudge origin away from plane
            owner->SetOrigin(trace.hitVector - (direction * 0.125f));
            owner->LinkArea();
        }

        // test if walking on steep slopes
        slope = trace.hitNormal.Dot(gravity);

        if(trace.hitTri && !trace.hitActor) {
            if(slope < 0 && slope >= -0.5f) {
                if(trace.hitTri == groundGeom) {
                    // remove vertical movement
                    slideNormal = (trace.hitNormal + (trace.hitNormal * gravity));
                    vel = (velocity + (velocity * gravity));

                    ImpactVelocity(vel, slideNormal, 1.024f);

                    // continue sliding down the slope
                    vel = (-gravity * velocity) + vel;
                    if(vel.Dot(velocity) <= 0) {
                        velocity.Clear();
                        break;
                    }

                    velocity = vel;
                    bCanStep = false;
                }
                else if(trace.hitMesh != NULL) {
                    // trying to move from ground to steep slope will be
                    // treated as a solid wall
                    cDir = trace.hitVector + (trace.hitNormal.Cross(-gravity).Normalize());

                    cDir += (cDir * gravity);
                    cDir -= gravity;

                    trace.hitVector += (trace.hitVector * gravity);
                    trace.hitVector -= gravity;

                    trace.hitNormal = trace.hitVector.Cross(cDir);
                    trace.hitNormal += (trace.hitNormal * gravity);
                    trace.hitNormal.Normalize();

                    bCanStep = false;
                }
            }
        }

        if(moves >= TRYMOVE_COUNT) {
            break;
        }

        normals[moves++] = trace.hitNormal;
        
        if(trace.hitTri && !trace.hitActor) {
            // handle stepping
            if(bCanStep && slope >= -0.5f) {
                trace.start = owner->GetOrigin();
                trace.end = trace.start + (-gravity * stepHeight);
                trace.dir = -gravity;

                // trace up
                localWorld.Trace(&trace);

                trace.start = trace.hitVector;
                trace.end = trace.start + (velocity * time);
                trace.dir = direction;

                // see if we can trace over the step
                localWorld.Trace(&trace);
                stepFraction = trace.fraction;

                trace.start = trace.hitVector;
                trace.end = trace.start + (gravity * stepHeight);
                trace.dir = gravity;

                // test the ground
                localWorld.Trace(&trace);
                slope = trace.hitNormal.Dot(-gravity);

                if(trace.hitTri != groundGeom && slope > 0.5f || trace.fraction >= 1) {
                    // don't try to step up against a wall
                    if(!(stepFraction < 0.99f && slope <= 0.5f)) {
                        owner->SetOrigin(trace.hitVector - (gravity * 0.125f));
                        owner->LinkArea();

                        if(stepFraction >= 1) {
                            break;
                        }

                        time -= (time * trace.fraction);
                    }
                }
            }
        }

        // try all interacted normals
        for(hits = 0; hits < moves; hits++) {
            if(velocity.Dot(normals[hits]) >= 0) {
                continue;
            }

            vel = velocity;
            ImpactVelocity(vel, normals[hits], 1.024f);

            // try bumping against another plane
            for(int j = 0; j < moves; j++) {
                if(j == hits || vel.Dot(normals[j]) >= 0) {
                    continue;
                }

                // bump into second plane
                ImpactVelocity(vel, normals[j], 1.024f);

                if(vel.Dot(normals[hits]) >= 0) {
                    continue;
                }

                // slide along the crease between two planes
                cDir = normals[hits].Cross(normals[j]).Normalize();
                d = cDir.Dot(velocity);
                vel = cDir * d;

                // see if it bumps into a third plane
                for(int k = 0; k < moves; k++) {
                    if(k != j && k != hits && vel.Dot(normals[k]) < 0) {
                        // force a dead stop
                        velocity.Clear();
                        return;
                    }
                }
            }

            velocity = vel;
            break;
        }
    }

    ApplyFriction();

    // correct position
    if(sector) {
        kexVec3 org = owner->GetOrigin();
        float dist = (org[1] - sector->lowerTri.GetDistance(org));

        if(dist < 0) {
            owner->GetOrigin()[1] = org[1] - dist;
            velocity.Clear();
        }

        // check if in water sector
        CheckWater((owner->GetViewHeight() + owner->GetCenterHeight()) * 0.5f);
    }
}

//
// kexPhysics::InitObject
//

void kexPhysics::InitObject(void) {
    scriptManager.Engine()->RegisterObjectType(
        "kPhysics",
        sizeof(kexPhysics),
        asOBJ_REF | asOBJ_NOCOUNT);

#define OBJMETHOD(str, a, b, c)                     \
    scriptManager.Engine()->RegisterObjectMethod(   \
        "kPhysics",                                 \
        str,                                        \
        asMETHODPR(kexPhysics, a, b, c),            \
        asCALL_THISCALL)

    OBJMETHOD("kActor @GetOwner(void)", GetOwner, (void), kexWorldObject*);
    OBJMETHOD("void SetOwner(kActor@)", SetOwner, (kexWorldObject *o), void);
    OBJMETHOD("kVec3 &GetVelocity(void)", GetVelocity, (void), kexVec3&);
    OBJMETHOD("void SetVelocity(const kVec3 &in)", SetVelocity, (const kexVec3 &vel), void);
    OBJMETHOD("bool OnGround(void)", OnGround, (void), bool);
    OBJMETHOD("bool OnSteepSlope(void)", OnSteepSlope, (void), bool);
    OBJMETHOD("float GroundDistance(void)", GroundDistance, (void), float);
    OBJMETHOD("float GetWaterDepth(void)", GetWaterDepth, (void), float);

#define OBJPROPERTY(str, p)                         \
    scriptManager.Engine()->RegisterObjectProperty( \
        "kPhysics",                                 \
        str,                                        \
        asOFFSET(kexPhysics, p))

    OBJPROPERTY("bool bRotor", bRotor);
    OBJPROPERTY("bool bOrientOnSlope", bOrientOnSlope);
    OBJPROPERTY("float friction", friction);
    OBJPROPERTY("float airFriction", airFriction);
    OBJPROPERTY("float mass", mass);
    OBJPROPERTY("float bounceDamp", bounceDamp);
    OBJPROPERTY("float stepHeight", stepHeight);
    OBJPROPERTY("float rotorSpeed", rotorSpeed);
    OBJPROPERTY("float rotorFriction", rotorFriction);
    OBJPROPERTY("kVec3 rotorVector", rotorVector);
    OBJPROPERTY("bool bInWater", bInWater);
    OBJPROPERTY("int waterLevel", waterLevel);
    OBJPROPERTY("float waterHeight", waterHeight);

#undef OBJMETHOD
#undef OBJPROPERTY

    scriptManager.Engine()->RegisterObjectMethod(
        "kActor",
        "kPhysics @Physics(void)",
        asMETHODPR(kexActor, Physics, (void), kexPhysics*),
        asCALL_THISCALL);
}
