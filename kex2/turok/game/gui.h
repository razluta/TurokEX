// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2014 Samuel Villarreal
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

#ifndef __GUI_H__
#define __GUI_H__

typedef enum {
    GUIS_DISABLED   = 0,
    GUIS_READY,
    GUIS_FADEIN,
    GUIS_FADEOUT,
    GUIS_NOTFOCUSED
} guiStatus_t;

typedef enum {
    GBS_NONE        = 0,
    GBS_DISABLED,
    GBS_HOVER,
    GBS_DOWN,
    GBS_PRESSED,
    GBS_RELEASED,
    GBS_EXITED
} guiButtonState_t;

typedef enum {
    GEVT_NONE       = 0,
    GEVT_ONDOWN,
    GEVT_ONRELEASE,
    NUMGUIEVENTS
} guiEventType_t;

typedef enum {
    GAT_NONE        = 0,
    GAT_CHANGEGUI,
    GAT_POPGUI,
    GAT_CALLCOMMAND,
    GAT_CALLFUNCTION,
    NUMGUIACTIONS
} guiActionType_t;

typedef struct {
    guiEventType_t                      event;
    guiActionType_t                     action;
    kexKeyMap                           args;
} guiEvent_t;

typedef struct guiButton_s {
    kexContainer                        *container;
    guiButtonState_t                    state;
    kexStr                              name;
    kexArray<guiEvent_t>                events;
    kexLinklist<struct guiButton_s>     link;
} guiButton_t;

typedef struct guiSlider_s {
    kexContainer                        *container;
    kexCanvasImage                      *barImage;
    kexCanvasImage                      *sliderImage;
    float                               barWidth;
    float                               position;
    bool                                bGrabbed;
    guiButtonState_t                    state;
    kexStr                              name;
    kexArray<guiEvent_t>                events;
    kexLinklist<struct guiSlider_s>     link;
} guiSlider_t;

class kexGui {
    friend class kexGuiManager;
public:
                                kexGui(void);
                                ~kexGui(void);
    
    void                        Draw(void);
    bool                        CheckEvents(const event_t *ev);
    void                        FadeIn(const float speed);
    void                        FadeOut(const float speed);
    
private:
    void                        UpdateSliders(void);
    void                        ExecuteEvent(guiEvent_t *event);
    void                        ExecuteButtonEvent(guiButton_t *button, const guiButtonState_t btnState);
    void                        ChangeGuis(guiEvent_t *guiEvent);
    void                        CallCommand(guiEvent_t *event);
    void                        CallFunction(guiEvent_t *event);
    
    kexCanvas                   canvas;
    kexLinklist<kexGui>         link;
    kexLinklist<guiButton_t>    buttons;
    kexLinklist<guiSlider_t>    sliders;
    kexStr                      name;
    guiStatus_t                 status;
    float                       fadeSpeed;
    kexGui                      *parentGui;
    kexGui                      *childGui;
};

#include "tinyxml2.h"

class kexGuiManager {
public:
                                kexGuiManager(void);
                                ~kexGuiManager(void);
    
    void                        Init(void);
    kexGui                      *LoadGui(const char *guiFile);
    kexGui                      *FindGuiByName(const char *name);
    void                        DrawGuis(void);
    void                        DeleteGuis(void);
    bool                        ProcessInput(const event_t *ev);
    void                        UpdateGuis(void);
    void                        ClearGuis(const float fadeSpeed);
    
    void                        Toggle(const bool bToggle) { bEnabled = bToggle; }
    const bool                  IsActive(void) const { return bEnabled; }
    void                        SetMainGui(kexGui *gui) { mainGui = gui; }
    kexGui                      *GetMainGui(void) { return mainGui; }
    
    kexLinklist<kexGui>         guis;
    kexGui                      *previousGui;
    float                       cursor_x;
    float                       cursor_y;
    kexMaterial                 *cursorMaterial;
    
    bool                        bDebugButtons;
    
private:
    void                        ParseColor(const char *colorString, byte &r, byte &g, byte &b);
    void                        ParseColor(tinyxml2::XMLElement *element, byte *rgb);
    void                        DrawCursor(void);
    void                        ParseNode(tinyxml2::XMLElement *element,
                                          kexGui *gui,
                                          kexContainer *container);
    void                        ParseSimpleProperties(tinyxml2::XMLNode *node,
                                                      kexCanvasObject *object);
    void                        ParseContainer(tinyxml2::XMLNode *node,
                                            kexGui *gui,
                                            kexContainer *container);
    void                        ParseImage(tinyxml2::XMLNode *node,
                                            kexGui *gui,
                                            kexContainer *container);
    void                        ParseText(tinyxml2::XMLNode *node,
                                            kexGui *gui,
                                            kexContainer *container);
    void                        ParseObject(tinyxml2::XMLNode *node,
                                            kexGui *gui,
                                            kexContainer *container);
    void                        ParseButton(tinyxml2::XMLNode *node,
                                            kexGui *gui,
                                            kexContainer *container);
    void                        ParseSlider(tinyxml2::XMLNode *node,
                                            kexGui *gui,
                                            kexContainer *container);
    void                        ParseButtonEvent(tinyxml2::XMLNode *node,
                                                 kexGui *gui,
                                                 kexContainer *container,
                                                 guiButton_t *button);
    
    bool                        bEnabled;
    kexGui                      *mainGui;
};

extern kexGuiManager guiManager;

#endif
