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
// DESCRIPTION: Model System
//
//-----------------------------------------------------------------------------

#include "common.h"
#include "kernel.h"
#include "zone.h"
#include "gl.h"
#include "client.h"
#include "script.h"
#include "render.h"
#include "mathlib.h"

static kmodel_t *mdl_hashlist[MAX_HASH];

enum
{
    scmdl_info = 0,
    scmdl_numanims,
    scmdl_animsets,
    scmdl_bbox,
    scmdl_types,
    scmdl_behaviors,
    scmdl_numbehaviors,
    scmdl_model,
    scmdl_numnodes,
    scmdl_nodes,
    scmdl_numchildren,
    scmdl_children,
    scmdl_numvariants,
    scmdl_variants,
    scmdl_numgroups,
    scmdl_groups,
    scmdl_numsections,
    scmdl_sections,
    scmdl_texture,
    scmdl_rgba1,
    scmdl_rgba2,
    scmdl_numtriangles,
    scmdl_triangles,
    scmdl_numvertices,
    scmdl_vertices,
    scmdl_xyz,
    scmdl_coords,
    scmdl_normals,
    scmdl_end
};

static const sctokens_t mdltokens[scmdl_end+1] =
{
    { scmdl_info,           "info"          },
    { scmdl_numanims,       "numanims"      },
    { scmdl_animsets,       "animsets"      },
    { scmdl_bbox,           "bbox"          },
    { scmdl_types,          "types"         },
    { scmdl_behaviors,      "behaviors"     },
    { scmdl_numbehaviors,   "numbehaviors"  },
    { scmdl_model,          "model"         },
    { scmdl_numnodes,       "numnodes"      },
    { scmdl_nodes,          "nodes"         },
    { scmdl_numchildren,    "numchildren"   },
    { scmdl_children,       "children"      },
    { scmdl_numvariants,    "numvariants"   },
    { scmdl_variants,       "variants"      },
    { scmdl_numgroups,      "numgroups"     },
    { scmdl_groups,         "groups"        },
    { scmdl_numsections,    "numsections"   },
    { scmdl_sections,       "sections"      },
    { scmdl_texture,        "texture"       },
    { scmdl_rgba1,          "rgba"          },
    { scmdl_rgba2,          "rgba2"         },
    { scmdl_numtriangles,   "numtriangles"  },
    { scmdl_triangles,      "triangles"     },
    { scmdl_numvertices,    "numvertices"   },
    { scmdl_vertices,       "vertices"      },
    { scmdl_xyz,            "xyz"           },
    { scmdl_coords,         "coords"        },
    { scmdl_normals,        "normals"       },
    { -1,                   NULL            }
};

enum
{
    scanim_anim = 0,
    scanim_numframes,
    scanim_numnodes,
    scanim_numtranslationsets,
    scanim_numrotationsets,
    scanim_nodeframes,
    scanim_initial_t,
    scanim_initial_r,
    scanim_translationsets,
    scanim_rotationsets,
    scanim_numactions,
    scanim_actions,
    scanim_initialtranslation,
    scanim_initialrotation,
    scanim_end
};

static const sctokens_t animtokens[scanim_end+1] =
{
    { scanim_anim,              "anim"                  },
    { scanim_numframes,         "numframes"             },
    { scanim_numnodes,          "numnodes"              },
    { scanim_numtranslationsets,"numtranslationsets"    },
    { scanim_numrotationsets,   "numrotationsets"       },
    { scanim_nodeframes,        "nodeframes"            },
    { scanim_initial_t,         "initialtranslation"    },
    { scanim_initial_r,         "initialrotation"       },
    { scanim_translationsets,   "translationsets"       },
    { scanim_rotationsets,      "rotationsets"          },
    { scanim_numactions,        "numactions"            },
    { scanim_actions,           "actions"               },
    { -1,                       NULL                    }
};

typedef struct
{
    const char  *name;
    int         flag;
} mdlflagnames_t;

static const mdlflagnames_t mdlflagnames[17] =
{
    { "unknown1",       1       },
    { "fullbright",     2       },
    { "nocullfaces",    4       },
    { "renderspecular", 8       },
    { "shinysurface",   16      },
    { "unknown32",      32      },
    { "solid",          64      },
    { "masked",         128     },
    { "transparent1",   256     },
    { "transparent2",   512     },
    { "colorize",       1024    },
    { "metalsurface",   2048    },
    { "unknown4096",    4096    },
    { "unknown8192",    8192    },
    { "unknown16384",   16384   },
    { "unknown32768",   32768   },
    { "unknown65536",   65536   }
};

//
// Mdl_ParseSectionBlock
//
// Majority of the model's geometry and data is contained in
// the section block
//

static void Mdl_ParseSectionBlock(mdlmesh_t *mesh, scparser_t *parser)
{
    unsigned int i;
    kbool nested = false;

    mesh->sections = (mdlsection_t*)Z_Calloc(sizeof(mdlsection_t) *
        mesh->numsections, PU_MODEL, 0);

    SC_ExpectTokenID(mdltokens, scmdl_sections, parser);
    SC_ExpectNextToken(TK_LBRACK);

    for(i = 0; i < mesh->numsections; i++)
    {
        mdlsection_t *section = &mesh->sections[i];

        // read into the nested section block
        SC_ExpectNextToken(TK_LBRACK);
        
        // this is the only block that doesn't require the fields
        // to be in a certain order
        while(parser->tokentype != TK_RBRACK || nested != false)
        {
            // required so the parser doesn't get confused with the
            // closing bracket of the section block and the nested
            // section blocks
            nested = false;

            SC_Find();

            switch(SC_GetIDForToken(mdltokens, parser->token))
            {
                // texture
            case scmdl_texture:
                SC_AssignString(mdltokens, section->texpath,
                    scmdl_texture, parser, false);
                break;
                // rgba
            case scmdl_rgba1:
                {
                    byte r, g, b, a;

                    SC_ExpectNextToken(TK_EQUAL);

                    r = SC_GetNumber();
                    g = SC_GetNumber();
                    b = SC_GetNumber();
                    a = SC_GetNumber();

                    section->color1 = RGBA(r, g, b, a);
                }
                break;
                // number of triangle indices
            case scmdl_numtriangles:
                SC_AssignInteger(mdltokens, &section->numtris,
                    scmdl_numtriangles, parser, false);
                section->numtris *= 3;
                break;
                // triangle indice array
            case scmdl_triangles:
                nested = true;
                SC_AssignArray(mdltokens, AT_SHORT, &section->tris, section->numtris,
                    scmdl_triangles, parser, false, PU_MODEL);
                break;
                // number of vertices
            case scmdl_numvertices:
                SC_AssignInteger(mdltokens, &section->numverts,
                    scmdl_numvertices, parser, false);
                break;
                // vertex block: contains nested arrays (vertices, tex coords and normals)
            case scmdl_vertices:
                nested = true;
                SC_ExpectNextToken(TK_EQUAL);
                SC_ExpectNextToken(TK_LBRACK);
                SC_AssignArray(mdltokens, AT_VECTOR, (void*)&section->xyz, section->numverts,
                    scmdl_xyz, parser, true, PU_MODEL);
                SC_AssignArray(mdltokens, AT_FLOAT, &section->coords, section->numverts * 2,
                    scmdl_coords, parser, true, PU_MODEL);
                SC_AssignArray(mdltokens, AT_FLOAT, (void*)&section->normals, section->numverts * 3,
                    scmdl_normals, parser, true, PU_MODEL);
                SC_ExpectNextToken(TK_RBRACK);
                break;
                // misc tokens will be considered as a flag or property name
            default:
                {
                    int j;

                    for(j = 0; j < 17; j++)
                    {
                        if(!strcmp(parser->token, mdlflagnames[j].name))
                        {
                            SC_ExpectNextToken(TK_EQUAL);
                            if(SC_GetNumber())
                            {
                                section->flags |= mdlflagnames[j].flag;
                                break;
                            }
                        }
                    }
                }
                break;
            }
        }
    }

    // end of section block
    SC_ExpectNextToken(TK_RBRACK);
}

//
// Mdl_ParseMeshBlock
//

static void Mdl_ParseMeshBlock(mdlnode_t *node, scparser_t *parser)
{
    unsigned int i;

    node->meshes = (mdlmesh_t*)Z_Calloc(sizeof(mdlmesh_t) * node->nummeshes, PU_MODEL, 0);

    SC_ExpectTokenID(mdltokens, scmdl_groups, parser);
    SC_ExpectNextToken(TK_LBRACK);

    for(i = 0; i < node->nummeshes; i++)
    {
        mdlmesh_t *mesh = &node->meshes[i];

        // read into the nested mesh block
        SC_ExpectNextToken(TK_LBRACK);
        SC_AssignInteger(mdltokens, &mesh->numsections,
            scmdl_numsections, parser, true);

        // read into section block
        Mdl_ParseSectionBlock(mesh, parser);

        // end of nested mesh block
        SC_ExpectNextToken(TK_RBRACK);
    }

    // end of mesh block
    SC_ExpectNextToken(TK_RBRACK);
}

//
// Mdl_ParseNodeBlock
//

static void Mdl_ParseNodeBlock(kmodel_t *model, scparser_t *parser)
{
    unsigned int i;

    model->nodes = (mdlnode_t*)Z_Calloc(sizeof(mdlnode_t) * model->numnodes, PU_MODEL, 0);

    for(i = 0; i < model->numnodes; i++)
    {
        mdlnode_t *node = &model->nodes[i];

        // read into nested node block
        SC_ExpectNextToken(TK_LBRACK);

        SC_AssignInteger(mdltokens, &node->numchildren,
            scmdl_numchildren, parser, true);

        if(node->numchildren > 0)
        {
            SC_AssignArray(mdltokens, AT_SHORT, &node->children, node->numchildren,
                scmdl_children, parser, true, PU_MODEL);
        }

        SC_AssignInteger(mdltokens, &node->numvariants,
            scmdl_numvariants, parser, true);

        // must have a variant
        if(node->numvariants <= 0)
        {
            SC_Error("Variant count for node %i should be at least 1 or more", i);
        }

        SC_AssignArray(mdltokens, AT_SHORT, &node->variants, node->numvariants,
            scmdl_variants, parser, true, PU_MODEL);
        SC_AssignInteger(mdltokens, &node->nummeshes,
            scmdl_numgroups, parser, true);

        // must have a mesh
        if(node->nummeshes <= 0)
        {
            SC_Error("Mesh count for node %i should be at least 1 or more", i);
        }

        // read into the mesh block
        Mdl_ParseMeshBlock(node, parser);

        // end of nested node block
        SC_ExpectNextToken(TK_RBRACK);
    }
}

//
// Mdl_ParseScript
//
// Main parsing routine
//

static void Mdl_ParseScript(kmodel_t *model, scparser_t *parser)
{
    unsigned int i;

    while(SC_CheckScriptState())
    {
        SC_Find();

        switch(parser->tokentype)
        {
        case TK_NONE:
            break;
        case TK_EOF:
            return;
        case TK_IDENIFIER:
            {
                // there are three main blocks for a kmesh file
                switch(SC_GetIDForToken(mdltokens, parser->token))
                {
                    // info block (bounding box, etc)
                case scmdl_info:
                    /*SC_ExpectNextToken(TK_LBRACK);
                    SC_ExpectTokenID(mdltokens, scmdl_bbox, parser);
                    SC_ExpectNextToken(TK_EQUAL);
                    SC_ExpectNextToken(TK_LBRACK);
                    model->bbox.min[0] = (float)SC_GetFloat();
                    model->bbox.min[1] = (float)SC_GetFloat();
                    model->bbox.min[2] = (float)SC_GetFloat();
                    model->bbox.max[0] = (float)SC_GetFloat();
                    model->bbox.max[1] = (float)SC_GetFloat();
                    model->bbox.max[2] = (float)SC_GetFloat();
                    SC_ExpectNextToken(TK_RBRACK);
                    SC_ExpectNextToken(TK_RBRACK);*/
                    break;
                    // behavior block (looping action, startup action, etc)
                case scmdl_behaviors:
                    break;
                    // model block (geometry)
                case scmdl_model:
                    SC_ExpectNextToken(TK_LBRACK);
                    SC_AssignInteger(mdltokens, &model->numnodes,
                        scmdl_numnodes, parser, true);

                    // begin reading into the node block
                    SC_ExpectTokenID(mdltokens, scmdl_nodes, parser);
                    SC_ExpectNextToken(TK_LBRACK);
                    Mdl_ParseNodeBlock(model, parser);

                    // end of node block
                    SC_ExpectNextToken(TK_RBRACK);

                    // end of model block
                    SC_ExpectNextToken(TK_RBRACK);
                    break;
                    // numanims variable
                case scmdl_numanims:
                    SC_ExpectNextToken(TK_EQUAL);
                    model->numanimations = SC_GetNumber();
                    break;
                    // animsets
                case scmdl_animsets:
                    SC_ExpectNextToken(TK_EQUAL);
                    SC_ExpectNextToken(TK_LBRACK);
                    if(model->numanimations > 0)
                    {
                        model->anims = (anim_t*)Z_Calloc(sizeof(anim_t) *
                            model->numanimations, PU_MODEL, 0);

                        for(i = 0; i < model->numanimations; i++)
                        {
                            SC_ExpectNextToken(TK_LBRACK);
                            SC_GetString();
                            model->anims[i].alias = Z_Strdup(sc_stringbuffer, PU_MODEL, 0);
                            SC_GetString();
                            memcpy(model->anims[i].animpath, sc_stringbuffer, MAX_FILEPATH);
                            SC_ExpectNextToken(TK_RBRACK);
                        }
                    }
                    SC_ExpectNextToken(TK_RBRACK);
                    break;
                default:
                    break;
                }
            }
            break;
        default:
            break;
        }
    }
}

//
// Mdl_ParseAnimScript
//

static void Mdl_ParseAnimScript(kmodel_t *model, anim_t *anim, scparser_t *parser)
{
    unsigned int numnodes;
    unsigned int i;
    unsigned int j;

    numnodes = 0;

    if(model->numnodes <= 0)
    {
        SC_Error("numnodes is 0 or has not been set yet for %s",
            model->mdlpath);
    }

    anim->frameset = (frameset_t*)Z_Calloc(sizeof(frameset_t)
        * model->numnodes, PU_MODEL, 0);

    SC_ExpectTokenID(animtokens, scanim_anim, parser);
    SC_ExpectNextToken(TK_LBRACK);

    while(SC_CheckScriptState())
    {
        SC_Find();

        switch(parser->tokentype)
        {
        case TK_NONE:
            return;
        case TK_EOF:
            return;
        case TK_IDENIFIER:
            {
                switch(SC_GetIDForToken(animtokens, parser->token))
                {
                    // frame count
                case scanim_numframes:
                    SC_AssignInteger(animtokens, &anim->numframes,
                        scanim_numframes, parser, false);
                    break;
                    // action count
                case scanim_numactions:
                    SC_AssignInteger(animtokens, &anim->numactions,
                        scanim_numactions, parser, false);
                    break;
                    // number of nodes (must match numnodes in model file)
                case scanim_numnodes:
                    SC_AssignInteger(animtokens, &numnodes,
                        scanim_numnodes, parser, false);

                    if(numnodes != model->numnodes)
                    {
                        SC_Error("numnodes(%i) for %s doesn't match numnodes in model file(%i)",
                            numnodes, anim->alias, model->numnodes);
                    }
                    break;
                    // translation table count
                case scanim_numtranslationsets:
                    SC_AssignInteger(animtokens, &anim->numtranslations,
                        scanim_numtranslationsets, parser, false);
                    break;
                    // rotation table count
                case scanim_numrotationsets:
                    SC_AssignInteger(animtokens, &anim->numrotations,
                        scanim_numrotationsets, parser, false);
                    break;
                    // translation table
                case scanim_translationsets:
                    SC_ExpectNextToken(TK_EQUAL);
                    if(anim->numtranslations <= 0)
                    {
                        SC_Error("numtranslations is 0 or has not been set yet for %s",
                            anim->alias);
                    }
                    if(anim->numframes <= 0)
                    {
                        SC_Error("numframes is 0 or has not been set yet for %s",
                            anim->alias);
                    }
                    anim->translations = (animtranslation_t**)Z_Calloc(sizeof(animtranslation_t*)
                        * anim->numtranslations, PU_MODEL, 0);

                    SC_ExpectNextToken(TK_LBRACK);
                    for(i = 0; i < anim->numtranslations; i++)
                    {
                        anim->translations[i] = (animtranslation_t*)Z_Calloc(
                            sizeof(animtranslation_t) * anim->numframes, PU_MODEL, 0);

                        SC_ExpectNextToken(TK_LBRACK);
                        for(j = 0; j < anim->numframes; j++)
                        {
                            SC_ExpectNextToken(TK_LBRACK);
                            anim->translations[i][j].vec[0] = (float)SC_GetFloat();
                            anim->translations[i][j].vec[1] = (float)SC_GetFloat();
                            anim->translations[i][j].vec[2] = (float)SC_GetFloat();
                            SC_ExpectNextToken(TK_RBRACK);
                        }
                        SC_ExpectNextToken(TK_RBRACK);
                    }
                    SC_ExpectNextToken(TK_RBRACK);
                    break;
                    // rotation table
                case scanim_rotationsets:
                    SC_ExpectNextToken(TK_EQUAL);
                    if(anim->numrotations <= 0)
                    {
                        SC_Error("numrotations is 0 or has not been set yet for %s",
                            anim->alias);
                    }
                    if(anim->numframes <= 0)
                    {
                        SC_Error("numframes is 0 or has not been set yet for %s",
                            anim->alias);
                    }
                    anim->rotations = (animrotation_t**)Z_Calloc(sizeof(animrotation_t*)
                        * anim->numrotations, PU_MODEL, 0);

                    SC_ExpectNextToken(TK_LBRACK);
                    for(i = 0; i < anim->numrotations; i++)
                    {
                        anim->rotations[i] = (animrotation_t*)Z_Calloc(
                            sizeof(animrotation_t) * anim->numframes, PU_MODEL, 0);

                        SC_ExpectNextToken(TK_LBRACK);
                        for(j = 0; j < anim->numframes; j++)
                        {
                            SC_ExpectNextToken(TK_LBRACK);
                            anim->rotations[i][j].vec[0] = (float)SC_GetFloat();
                            anim->rotations[i][j].vec[1] = (float)SC_GetFloat();
                            anim->rotations[i][j].vec[2] = (float)SC_GetFloat();
                            anim->rotations[i][j].vec[3] = (float)SC_GetFloat();
                            SC_ExpectNextToken(TK_RBRACK);
                        }
                        SC_ExpectNextToken(TK_RBRACK);
                    }
                    SC_ExpectNextToken(TK_RBRACK);
                    break;
                    // lookup table for model nodes
                case scanim_nodeframes:
                    SC_ExpectNextToken(TK_EQUAL);
                    SC_ExpectNextToken(TK_LBRACK);
                    for(i = 0; i < model->numnodes; i++)
                    {
                        int num;

                        SC_ExpectNextToken(TK_LBRACK);

                        num = SC_GetNumber();
                        anim->frameset[i].translation = num != -1 ?
                            anim->translations[num] : NULL;

                        num = SC_GetNumber();
                        anim->frameset[i].rotation = num != -1 ?
                            anim->rotations[num] : NULL;

                        SC_ExpectNextToken(TK_RBRACK);
                    }
                    SC_ExpectNextToken(TK_RBRACK);
                    break;
                    // actions
                /*case scanim_actions:
                    if(anim->numactions <= 0)
                    {
                        SC_Error("numactions is 0 or has not been set yet for %s",
                            anim->alias);
                    }
                    SC_ExpectNextToken(TK_EQUAL);
                    break;*/
                    // initial translation frame
                case scanim_initial_t:
                    SC_ExpectNextToken(TK_EQUAL);
                    SC_ExpectNextToken(TK_LBRACK);

                    anim->initial.translation = (animtranslation_t*)Z_Calloc(sizeof(animtranslation_t)
                        * model->numnodes, PU_MODEL, 0);

                    for(i = 0; i < model->numnodes; i++)
                    {
                        SC_ExpectNextToken(TK_LBRACK);
                        anim->initial.translation[i].vec[0] = (float)SC_GetFloat();
                        anim->initial.translation[i].vec[1] = (float)SC_GetFloat();
                        anim->initial.translation[i].vec[2] = (float)SC_GetFloat();
                        SC_ExpectNextToken(TK_RBRACK);
                    }

                    SC_ExpectNextToken(TK_RBRACK);
                    break;
                    // initial rotation frame
                case scanim_initial_r:
                    SC_ExpectNextToken(TK_EQUAL);
                    SC_ExpectNextToken(TK_LBRACK);

                    anim->initial.rotation = (animrotation_t*)Z_Calloc(sizeof(animrotation_t)
                        * model->numnodes, PU_MODEL, 0);

                    for(i = 0; i < model->numnodes; i++)
                    {
                        SC_ExpectNextToken(TK_LBRACK);
                        anim->initial.rotation[i].vec[0] = (float)SC_GetFloat();
                        anim->initial.rotation[i].vec[1] = (float)SC_GetFloat();
                        anim->initial.rotation[i].vec[2] = (float)SC_GetFloat();
                        anim->initial.rotation[i].vec[3] = (float)SC_GetFloat();
                        SC_ExpectNextToken(TK_RBRACK);
                    }

                    SC_ExpectNextToken(TK_RBRACK);
                    break;
                default:
                    if(parser->tokentype == TK_IDENIFIER)
                    {
                        Com_DPrintf("Mdl_ParseAnimScript: Unknown token: %s\n",
                            parser->token);
                    }
                    break;
                }
            }
            break;
        default:
            break;
        }
    }

    SC_ExpectNextToken(TK_RBRACK);
}

//
// Mdl_LoadAnimations
//

static void Mdl_LoadAnimations(kmodel_t *model)
{
    unsigned int i;

    if(model->anims == NULL || model->numanimations <= 0 || model->numnodes <= 0)
    {
        return;
    }

    for(i = 0; i < model->numanimations; i++)
    {
        scparser_t *parser;

        if(!(parser = SC_Open(model->anims[i].animpath)))
        {
            continue;
        }

        Mdl_ParseAnimScript(model, &model->anims[i], parser);
        SC_Close();
    }
}

//
// Mdl_GetAnim
//

anim_t *Mdl_GetAnim(kmodel_t *model, const char *name)
{
    unsigned int i;

    if(model->anims == NULL || model->numanimations <= 0)
    {
        return NULL;
    }

    for(i = 0; i < model->numanimations; i++)
    {
        if(!strcmp(model->anims[i].alias, name))
        {
            return &model->anims[i];
        }
    }

    return NULL;
}

//
// Mdl_SetAnimState
//

void Mdl_SetAnimState(animstate_t *astate, anim_t *anim,
                      float time, animflags_t flags)
{
    astate->time                    = (float)client.tics + time;
    astate->deltatime               = 0;
    astate->blendtime               = 0;
    astate->frametime               = time;
    astate->track.frame             = 1;
    astate->track.nextframe         = 2;
    astate->flags                   = flags;
    astate->prevtrack.frame         = 0;
    astate->prevtrack.nextframe     = 0;
    astate->track.anim              = anim;
    astate->prevtrack.anim          = NULL;
}


//
// Mdl_BlendAnimStates
//

void Mdl_BlendAnimStates(animstate_t *astate, anim_t *anim,
                         float time, float blendtime, animflags_t flags)
{
    if(astate->flags & (ANF_BLEND|ANF_NOINTERRUPT))
    {
        // abort if already blending
        return;
    }

    if(anim != astate->track.anim)
    {
        if(astate->prevtrack.anim == anim)
        {
            // don't blend the same anim
            return;
        }

        astate->flags                   = flags | ANF_BLEND;
        astate->prevtrack.frame         = astate->track.frame;
        astate->prevtrack.nextframe     = astate->track.nextframe;
        astate->track.frame             = 1;
        astate->track.nextframe         = 2;
        astate->time                    = (float)client.tics + blendtime;
        astate->frametime               = time;
        astate->blendtime               = blendtime;
        astate->deltatime               = 0;
        astate->prevtrack.anim          = astate->track.anim;
        astate->track.anim              = anim;
    }
}


//
// Mdl_UpdateAnimState
//

void Mdl_UpdateAnimState(animstate_t *astate)
{
    if(astate->flags & ANF_STOPPED)
        return;

    if(astate->time <= client.tics)
    {
        astate->deltatime = 0;
        astate->time = (float)client.tics + astate->frametime;

        if(astate->flags & ANF_BLEND)
        {
            astate->prevtrack.anim = NULL;
            astate->blendtime = 0;
            astate->flags &= ~ANF_BLEND;
        }

        if(++astate->track.frame >=
            (int)astate->track.anim->numframes)
        {
            astate->track.frame = 1;
        }

        if(++astate->track.nextframe >=
            (int)astate->track.anim->numframes)
        {
            astate->track.nextframe = 1;

            if(!(astate->flags & ANF_LOOP))
            {
                astate->flags |= ANF_STOPPED;
                astate->flags &= ~ANF_NOINTERRUPT;
            }
        }
    }
    else
    {
        float blend;

        blend = (astate->flags & ANF_BLEND) ?
            astate->blendtime : astate->frametime;

        astate->deltatime += (1/blend);
    }
}

//
// Mdl_GetAnimRotation
//

void Mdl_GetAnimRotation(vec4_t out, anim_t *anim, int nodenum, int frame)
{
    if(anim->frameset[nodenum].rotation == NULL)
    {
        Vec_Copy4(out, anim->initial.rotation[nodenum].vec);
        return;
    }
    
    Vec_Copy4(out, anim->frameset[nodenum].rotation[frame].vec);
}

//
// Mdl_GetAnimTranslation
//

void Mdl_GetAnimTranslation(vec3_t out, anim_t *anim, int nodenum, int frame)
{
    if(anim->frameset[nodenum].translation == NULL)
    {
        Vec_Copy3(out, anim->initial.translation[nodenum].vec);
        return;
    }
    
    Vec_Copy3(out, anim->frameset[nodenum].translation[frame].vec);
}

//
// Mdl_Find
//

kmodel_t *Mdl_Find(const char *name)
{
    kmodel_t *model;
    unsigned int hash;

    hash = Com_HashFileName(name);

    for(model = mdl_hashlist[hash]; model; model = model->next)
    {
        if(!strcmp(name, model->mdlpath))
        {
            return model;
        }
    }

    return NULL;
}

//
// Mdl_Load
//
// Loads a kmesh file and parses the contents
//

kmodel_t *Mdl_Load(const char *file)
{
    kmodel_t *model;

    if(file == NULL)
    {
        return NULL;
    }
    else if(file[0] == 0)
    {
        return NULL;
    }

    // is the model already parsed/allocated?
    if(!(model = Mdl_Find(file)))
    {
        scparser_t *parser;
        unsigned int hash;

        if(!(parser = SC_Open(file)))
        {
            return NULL;
        }

        model = (kmodel_t*)Z_Calloc(sizeof(kmodel_t), PU_MODEL, 0);
        strncpy(model->mdlpath, file, MAX_FILEPATH);

        // begin parsing
        Mdl_ParseScript(model, parser);

        // add to hash for future reference
        hash = Com_HashFileName(model->mdlpath);
        model->next = mdl_hashlist[hash];
        mdl_hashlist[hash] = model;

        // we're done with the file
        SC_Close();

        Mdl_LoadAnimations(model);
    }

    return model;
}

//
// FCmd_LoadTestModel
//

static void FCmd_LoadTestModel(void)
{
    kmodel_t *model;
    int time;

    if(Cmd_GetArgc() < 2)
    {
        return;
    }

    time = Sys_GetMilliseconds();

    model = Mdl_Load(Cmd_GetArgv(1));
    if(model == NULL)
    {
        return;
    }

    Com_DPrintf("\nloadtime: %f seconds\n\n",
        (float)(Sys_GetMilliseconds() - time) / 1000.0f);
}

//
// Mdl_Init
//

void Mdl_Init(void)
{
    Cmd_AddCommand("loadmodel", FCmd_LoadTestModel);
}
