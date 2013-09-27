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
//
// DESCRIPTION: Clip meshes
//
//-----------------------------------------------------------------------------

#include "common.h"
#include "zone.h"
#include "clipmesh.h"
#include "renderSystem.h"

//
// kexClipMesh::kexClipMesh
//

kexClipMesh::kexClipMesh(void) {
    this->numPoints     = 0;
    this->points        = NULL;
    this->numIndices    = 0;
    this->indices       = NULL;
    this->numTriangles  = 0;
    this->triangles     = NULL;

    this->origin.Set(0, 0, 0);
}

//
// kexClipMesh::~kexClipMesh
//

kexClipMesh::~kexClipMesh(void) {
    this->numPoints     = 0;
    this->numIndices    = 0;
    this->numTriangles  = 0;
    
    if(this->points) {
        Z_Free(this->points);
    }
    if(this->indices) {
        Z_Free(this->indices);
    }
    if(this->triangles) {
        Z_Free(this->triangles);
    }
}

//
// kexClipMesh::CreateBox
//

void kexClipMesh::CreateBox(const kexBBox &bbox) {
    numPoints       = 8;
    numIndices      = 36;
    numTriangles    = 12;
    origin          = bbox.Center();
    points          = (kexVec3*)Z_Calloc(sizeof(kexVec3) * numPoints, PU_CM, NULL);
    indices         = (word*)Z_Calloc(sizeof(word) * numIndices, PU_CM, NULL);
    triangles       = (kexTri*)Z_Calloc(sizeof(kexTri) * numTriangles, PU_CM, NULL);
    
    indices[ 0] = 0; indices[ 1] = 1; indices[ 2] = 3;
    indices[ 3] = 4; indices[ 4] = 7; indices[ 5] = 5;
    indices[ 6] = 0; indices[ 7] = 4; indices[ 8] = 1;
    indices[ 9] = 1; indices[10] = 5; indices[11] = 6;
    indices[12] = 2; indices[13] = 6; indices[14] = 7;
    indices[15] = 4; indices[16] = 0; indices[17] = 3;
    indices[18] = 1; indices[19] = 2; indices[20] = 3;
    indices[21] = 7; indices[22] = 6; indices[23] = 5;
    indices[24] = 2; indices[25] = 1; indices[26] = 6;
    indices[27] = 3; indices[28] = 2; indices[29] = 7;
    indices[30] = 7; indices[31] = 4; indices[32] = 3;
    indices[33] = 4; indices[34] = 5; indices[35] = 1;
    
    points[0].x = bbox.max[0];
    points[0].y = bbox.min[1];
    points[0].z = bbox.min[2];
    points[1].x = bbox.max[0];
    points[1].y = bbox.min[1];
    points[1].z = bbox.max[2];
    points[2].x = bbox.min[0];
    points[2].y = bbox.min[1];
    points[2].z = bbox.max[2];
    points[3]   = bbox.min;
    points[4].x = bbox.max[0];
    points[4].y = bbox.max[1];
    points[4].z = bbox.min[2];
    points[5]   = bbox.max;
    points[6].x = bbox.min[0];
    points[6].y = bbox.max[1];
    points[6].z = bbox.max[2];
    points[7].x = bbox.min[0];
    points[7].y = bbox.max[1];
    points[7].z = bbox.min[2];
    
    for(unsigned int i = 0; i < numTriangles; i++) {
        triangles[i].point[0] = &points[indices[i * 3 + 0]];
        triangles[i].point[1] = &points[indices[i * 3 + 1]];
        triangles[i].point[2] = &points[indices[i * 3 + 2]];
        triangles[i].plane.SetNormal(
            *triangles[i].point[0],
            *triangles[i].point[1],
            *triangles[i].point[2]);
        triangles[i].plane.SetDistance(*triangles[i].point[0]);
    }
}

//
// kexClipMesh::CreateTetrahedron
//

void kexClipMesh::CreateTetrahedron(const kexBBox &bbox) {
    float c1 = 0.4714045207f;
    float c2 = 0.8164965809f;
    float c3 = -0.3333333333f;
    kexVec3 s;

    numPoints       = 4;
    numIndices      = 12;
    numTriangles    = 4;
    origin          = bbox.Center();
    s               = bbox.max - origin;
    points          = (kexVec3*)Z_Calloc(sizeof(kexVec3) * numPoints, PU_CM, NULL);
    indices         = (word*)Z_Calloc(sizeof(word) * numIndices, PU_CM, NULL);
    triangles       = (kexTri*)Z_Calloc(sizeof(kexTri) * numTriangles, PU_CM, NULL);

    indices[ 0] = 0; indices[ 1] = 1; indices[ 2] = 2;
    indices[ 3] = 0; indices[ 4] = 2; indices[ 5] = 3;
    indices[ 6] = 0; indices[ 7] = 3; indices[ 8] = 1;
    indices[ 9] = 1; indices[10] = 3; indices[11] = 2;

    points[0] = origin + kexVec3(0, 0, s.z);
    points[1] = origin + kexVec3(2.0f * c1 * s.x, 0, c3 * s.z);
    points[2] = origin + kexVec3(-c1 * s.x, c2 * s.y, c3 * s.z);
    points[3] = origin + kexVec3(-c1 * s.x, -c2 * s.y, c3 * s.z);

    for(unsigned int i = 0; i < numTriangles; i++) {
        triangles[i].point[0] = &points[indices[i * 3 + 0]];
        triangles[i].point[1] = &points[indices[i * 3 + 1]];
        triangles[i].point[2] = &points[indices[i * 3 + 2]];
        triangles[i].plane.SetNormal(
            *triangles[i].point[0],
            *triangles[i].point[1],
            *triangles[i].point[2]);
        triangles[i].plane.SetDistance(*triangles[i].point[0]);
    }
}

//
// kexClipMesh::CreateOctahedron
//

void kexClipMesh::CreateOctahedron(const kexBBox &bbox) {
    kexVec3 s;

    numPoints       = 6;
    numIndices      = 24;
    numTriangles    = 8;
    origin          = bbox.Center();
    s               = bbox.max - origin;
    points          = (kexVec3*)Z_Calloc(sizeof(kexVec3) * numPoints, PU_CM, NULL);
    indices         = (word*)Z_Calloc(sizeof(word) * numIndices, PU_CM, NULL);
    triangles       = (kexTri*)Z_Calloc(sizeof(kexTri) * numTriangles, PU_CM, NULL);

    indices[ 0] = 4; indices[ 1] = 0; indices[ 2] = 2;
    indices[ 3] = 4; indices[ 4] = 2; indices[ 5] = 1;
    indices[ 6] = 4; indices[ 7] = 1; indices[ 8] = 3;
    indices[ 9] = 4; indices[10] = 3; indices[11] = 0;
    indices[12] = 5; indices[13] = 2; indices[14] = 0;
    indices[15] = 5; indices[16] = 1; indices[17] = 2;
    indices[18] = 5; indices[19] = 3; indices[20] = 1;
    indices[21] = 5; indices[22] = 0; indices[23] = 3;

    points[0] = origin + kexVec3(s.x, 0, 0);
    points[1] = origin + kexVec3(-s.x, 0, 0);
    points[2] = origin + kexVec3(0, s.y, 0);
    points[3] = origin + kexVec3(0, -s.y, 0);
    points[4] = origin + kexVec3(0, 0, s.z);
    points[5] = origin + kexVec3(0, 0, -s.z);

    for(unsigned int i = 0; i < numTriangles; i++) {
        triangles[i].point[0] = &points[indices[i * 3 + 0]];
        triangles[i].point[1] = &points[indices[i * 3 + 1]];
        triangles[i].point[2] = &points[indices[i * 3 + 2]];
        triangles[i].plane.SetNormal(
            *triangles[i].point[0],
            *triangles[i].point[1],
            *triangles[i].point[2]);
        triangles[i].plane.SetDistance(*triangles[i].point[0]);
    }
}

//
// kexClipMesh::CreateDodecahedron
//

void kexClipMesh::CreateDodecahedron(const kexBBox &bbox) {
    numPoints       = 20;
    numIndices      = 108;
    numTriangles    = 36;
    origin          = bbox.Center();
    points          = (kexVec3*)Z_Calloc(sizeof(kexVec3) * numPoints, PU_CM, NULL);
    indices         = (word*)Z_Calloc(sizeof(word) * numIndices, PU_CM, NULL);
    triangles       = (kexTri*)Z_Calloc(sizeof(kexTri) * numTriangles, PU_CM, NULL);

    indices[ 0] = 0;   indices[ 1] = 8;  indices[ 2] = 4;
    indices[ 3] = 8;   indices[ 4] = 9;  indices[ 5] = 4;
    indices[ 6] = 0;   indices[ 7] = 12; indices[ 8] = 8;
    indices[ 9] = 12;  indices[10] = 13; indices[11] = 1;
    indices[12] = 0;   indices[13] = 16; indices[14] = 2;
    indices[15] = 16;  indices[16] = 17; indices[17] = 2;
    indices[18] = 8;   indices[19] = 1;  indices[20] = 5;
    indices[21] = 1;   indices[22] = 18; indices[23] = 5;
    indices[24] = 12;  indices[25] = 2;  indices[26] = 3;
    indices[27] = 2;   indices[28] = 10; indices[29] = 3;
    indices[30] = 16;  indices[31] = 4;  indices[32] = 6;
    indices[33] = 4;   indices[34] = 14; indices[35] = 6;
    indices[36] = 9;   indices[37] = 5;  indices[38] = 14;
    indices[39] = 5;   indices[40] = 15; indices[41] = 14;
    indices[42] = 6;   indices[43] = 11; indices[44] = 2;
    indices[45] = 11;  indices[46] = 10; indices[47] = 2;
    indices[48] = 3;   indices[49] = 19; indices[50] = 1;
    indices[51] = 19;  indices[52] = 18; indices[53] = 1;
    indices[54] = 7;   indices[55] = 15; indices[56] = 18;
    indices[57] = 15;  indices[58] = 5;  indices[59] = 18;
    indices[60] = 7;   indices[61] = 11; indices[62] = 14;
    indices[63] = 11;  indices[64] = 6;  indices[65] = 14;
    indices[66] = 7;   indices[67] = 19; indices[68] = 10;
    indices[69] = 19;  indices[70] = 3;  indices[71] = 10;
    indices[72] = 16;  indices[73] = 0;  indices[74] = 4;
    indices[75] = 12;  indices[76] = 0;  indices[77] = 2;
    indices[78] = 9;   indices[79] = 8;  indices[80] = 5;
    indices[81] = 13;  indices[82] = 12; indices[83] = 3;
    indices[84] = 17;  indices[85] = 16; indices[86] = 6;
    indices[87] = 4;   indices[88] = 9;  indices[89] = 14;
    indices[90] = 17;  indices[91] = 6;  indices[92] = 2;
    indices[93] = 13;  indices[94] = 3;  indices[95] = 1;
    indices[96] = 19;  indices[97] = 7;  indices[98] = 18;
    indices[99] = 15;  indices[100] = 7; indices[101] = 14;
    indices[102] = 11; indices[103] = 7; indices[104] = 10;
    indices[105] = 12; indices[106] = 1; indices[107] = 8;

    float a[3], b[3], c[3];
    float s;

    a[0] = a[1] = a[2] = 0.5773502691896257f;
    b[0] = b[1] = b[2] = 0.3568220897730899f;
    c[0] = c[1] = c[2] = 0.9341723589627156f;

    float d = 0.5f / c[0];

    s = (bbox.max.x - bbox.min.x) * d;
    a[0] *= s;
    a[1] *= s;
    a[2] *= s;
    s = (bbox.max.y - bbox.min.y) * d;
    b[0] *= s;
    b[1] *= s;
    b[2] *= s;
    s = (bbox.max.z - bbox.min.z) * d;
    c[0] *= s;
    c[1] *= s;
    c[2] *= s;

    points[ 0].Set(origin[0] + a[0], origin[1] + a[1], origin[2] - a[2]);
    points[ 1].Set(origin[0] + a[0], origin[1] - a[1], origin[2] - a[2]);
    points[ 2].Set(origin[0] + a[0], origin[1] + a[1], origin[2] + a[2]);
    points[ 3].Set(origin[0] + a[0], origin[1] - a[1], origin[2] + a[2]);
    points[ 4].Set(origin[0] - a[0], origin[1] + a[1], origin[2] - a[2]);
    points[ 5].Set(origin[0] - a[0], origin[1] - a[1], origin[2] - a[2]);
    points[ 6].Set(origin[0] - a[0], origin[1] + a[1], origin[2] + a[2]);
    points[ 7].Set(origin[0] - a[0], origin[1] - a[1], origin[2] + a[2]);
    points[ 8].Set(origin[0] + b[0], origin[1]       , origin[2] - c[2]);
    points[ 9].Set(origin[0] - b[0], origin[1]       , origin[2] - c[2]);
    points[10].Set(origin[0] + b[0], origin[1]       , origin[2] + c[2]);
    points[11].Set(origin[0] - b[0], origin[1]       , origin[2] + c[2]);
    points[12].Set(origin[0] + c[2], origin[1] + b[1], origin[2]);
    points[13].Set(origin[0] + c[2], origin[1] - b[1], origin[2]);
    points[14].Set(origin[0] - c[2], origin[1] + b[1], origin[2]);
    points[15].Set(origin[0] - c[2], origin[1] - b[1], origin[2]);
    points[16].Set(origin[0]       , origin[1] + c[1], origin[2] - b[0]);
    points[17].Set(origin[0]       , origin[1] + c[1], origin[2] + b[0]);
    points[18].Set(origin[0]       , origin[1] - c[1], origin[2] - b[0]);
    points[19].Set(origin[0]       , origin[1] - c[1], origin[2] + b[0]);

    for(unsigned int i = 0; i < numTriangles; i++) {
        triangles[i].point[0] = &points[indices[i * 3 + 0]];
        triangles[i].point[1] = &points[indices[i * 3 + 1]];
        triangles[i].point[2] = &points[indices[i * 3 + 2]];
        triangles[i].plane.SetNormal(
            *triangles[i].point[0],
            *triangles[i].point[1],
            *triangles[i].point[2]);
        triangles[i].plane.SetDistance(*triangles[i].point[0]);
    }
}

//
// kexClipMesh::DebugDraw
//

void kexClipMesh::DebugDraw(void) {
    renderSystem.SetState(GLSTATE_TEXTURE0, false);
    renderSystem.SetState(GLSTATE_BLEND, true);
    renderSystem.SetState(GLSTATE_ALPHATEST, true);
    renderSystem.SetState(GLSTATE_LIGHTING, false);

    dglColor4ub(0xFF, 0, 0, 128);

    dglDisableClientState(GL_NORMAL_ARRAY);
    dglDisableClientState(GL_TEXTURE_COORD_ARRAY);

    dglVertexPointer(3, GL_FLOAT, sizeof(kexVec3), reinterpret_cast<float*>(&points[0].x));
    dglDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, indices);

    dglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    dglColor4ub(0xFF, 0xFF, 0xFF, 0xFF);
    dglDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, indices);
    dglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    dglEnableClientState(GL_NORMAL_ARRAY);
    dglEnableClientState(GL_TEXTURE_COORD_ARRAY);

    renderSystem.SetState(GLSTATE_TEXTURE0, true);
    renderSystem.SetState(GLSTATE_BLEND, false);
    renderSystem.SetState(GLSTATE_ALPHATEST, false);
    renderSystem.SetState(GLSTATE_LIGHTING, true);
}