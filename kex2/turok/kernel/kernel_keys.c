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
// DESCRIPTION: Key input handling and binding
//
//-----------------------------------------------------------------------------

#include "SDL.h"

#include "common.h"
#include "kernel.h"
#include "zone.h"

char keycode[2][MAX_KEYS];

control_t control;

typedef struct cmdlist_s
{
    char *command;
    struct cmdlist_s *next;
} cmdlist_t;

typedef struct
{
    cmdlist_t *cmds;
    char *name;
} keycmd_t;

static keycmd_t keycmds[MAX_KEYS];
static kbool shiftdown = false;
static kbool keydown[MAX_KEYS];

typedef struct
{
    int		code;
    char	*name;
} keyinfo_t;

static keyinfo_t Keys[] =
{
    { SDLK_RIGHT,       "right" },
    { SDLK_LEFT,        "left" },
    { SDLK_UP,          "up" },
    { SDLK_DOWN,        "down" },
    { SDLK_ESCAPE,      "escape" },
    { SDLK_RETURN,      "enter" },
    { SDLK_TAB,         "tab" },
    { SDLK_BACKSPACE,   "backsp" },
    { SDLK_PAUSE,       "pause" },
    { SDLK_LSHIFT,      "shift" },
    { SDLK_LALT,        "alt" },
    { SDLK_LCTRL,       "ctrl" },
    { SDLK_PLUS,        "+" },
    { SDLK_MINUS,       "-" },
    { SDLK_CAPSLOCK,    "caps" },
    { SDLK_INSERT,      "ins" },
    { SDLK_DELETE,      "del" },
    { SDLK_HOME,        "home" },
    { SDLK_END,         "end" },
    { SDLK_PAGEUP,      "pgup" },
    { SDLK_PAGEDOWN,    "pgdn" },
    { SDLK_SPACE,       "space" },
    { SDLK_F1,          "f1" },
    { SDLK_F2,          "f2" },
    { SDLK_F3,          "f3" },
    { SDLK_F4,          "f4" },
    { SDLK_F5,          "f5" },
    { SDLK_F6,          "f6" },
    { SDLK_F7,          "f7" },
    { SDLK_F8,          "f8" },
    { SDLK_F9,          "f9" },
    { SDLK_F10,         "f10" },
    { SDLK_F11,         "f11" },
    { SDLK_F12,         "f12" },
    { SDLK_KP_ENTER,    "keypadenter" },
    { SDLK_KP_MULTIPLY, "keypad*" },
    { SDLK_KP_PLUS,     "keypad+" },
    { SDLK_NUMLOCK,     "numlock" },
    { SDLK_KP_MINUS,    "keypad-" },
    { SDLK_KP_PERIOD,   "keypad." },
    { SDLK_KP_DIVIDE,   "keypad/" },
    { SDLK_KP0,         "keypad0" },
    { SDLK_KP1,         "keypad1" },
    { SDLK_KP2,         "keypad2" },
    { SDLK_KP3,         "keypad3" },
    { SDLK_KP4,         "keypad4" },
    { SDLK_KP5,         "keypad5" },
    { SDLK_KP6,         "keypad6" },
    { SDLK_KP7,         "keypad7" },
    { SDLK_KP8,         "keypad8" },
    { SDLK_KP9,         "keypad9" },
    { 0,                NULL }
};

//
// Key_GetKeyCode
//

static int Key_GetKeyCode(char *key)
{
    keyinfo_t *pkey;
    int len;

    strlwr(key);
    len = strlen(key);

    if(len == 1)
    {
        if(((*key >= 'a') && (*key <= 'z')) || ((*key >= '0') && (*key <= '9')))
        {
            return *key;
        }
    }

    for(pkey = Keys; pkey->name; pkey++)
    {
        if(!strcmp(key, pkey->name))
        {
            return pkey->code;
        }
    }

    return 0;
}

//
// Key_GetName
//

int Key_GetName(char *buff, int key)
{
    keyinfo_t *pkey;
        
    if(((key >= 'a') && (key <= 'z')) || ((key >= '0') && (key <= '9')))
    {
        buff[0] = (char)toupper(key);
        buff[1] = 0;

        return true;
    }
    for(pkey = Keys; pkey->name; pkey++)
    {
        if(pkey->code == key)
        {
            strcpy(buff, pkey->name);
            return true;
        }
    }
    sprintf(buff, "Key%02x", key);
    return false;
}


//
// Key_BindCmd
//

void Key_BindCmd(char key, const char *string)
{
    keycmd_t *keycmd;
    cmdlist_t *newcmd;

    keycmd = &keycmds[keycode[shiftdown][key]];
    newcmd = (cmdlist_t*)Z_Calloc(sizeof(cmdlist_t), PU_STATIC, 0);
    newcmd->command = Z_Strdup(string, PU_STATIC, 0);
    newcmd->next = keycmd->cmds;
    keycmd->cmds = newcmd;
}

//
// Key_WriteBindings
//

void Key_WriteBindings(FILE *file)
{
    keycmd_t *keycmd;
    cmdlist_t *cmd;
    int i;

    for(i = 0; i < MAX_KEYS; i++)
    {
        keycmd = &keycmds[i];

        for(cmd = keycmd->cmds; cmd; cmd = cmd->next)
        {
            char buff[32];

            Key_GetName(buff, i);
            strlwr(buff);

            fprintf(file, "bind %s \"%s\"\n", buff, cmd->command);
        }
    }
}

//
// Key_ExecCmd
//

void Key_ExecCmd(char key, kbool keyup)
{
    keycmd_t *keycmd;
    cmdlist_t *cmd;

    keycmd = &keycmds[keycode[shiftdown][key]];

    for(cmd = keycmd->cmds; cmd; cmd = cmd->next)
    {
        if(keyup && cmd->command[0] == '+')
        {
            cmd->command[0] = '-';
        }

        Cmd_ExecuteCommand(cmd->command);

        if(keyup && cmd->command[0] == '-')
        {
            cmd->command[0] = '+';
        }
    }
}

//
// Key_ClearControls
//

void Key_ClearControls(void)
{
    memset(&control, 0, sizeof(control_t));
}

//
// Key_HandleControl
//

static void Key_HandleControl(int ctrl)
{
    int ctrlkey;
    
    ctrlkey = (ctrl & CKF_COUNTMASK);
    
    if(ctrl & CKF_UP)
    {
        if((control.key[ctrlkey] & CKF_COUNTMASK) > 0)
        {
            control.key[ctrlkey]--;
        }
    }
    else
    {
        control.key[ctrlkey]++;
    }
}

#define CONTROL_KEY(name, data)                                             \
    static void FCmd_ ## name ## Down(void) { Key_HandleControl(data); }    \
    static void FCmd_ ## name ## Up(void) { Key_HandleControl(data|CKF_UP); }

CONTROL_KEY(Attack,         KEY_ATTACK);
CONTROL_KEY(Forward,        KEY_FORWARD);
CONTROL_KEY(Back,           KEY_BACK);
CONTROL_KEY(Left,           KEY_LEFT);
CONTROL_KEY(Right,          KEY_RIGHT);
CONTROL_KEY(StrafeLeft,     KEY_STRAFELEFT);
CONTROL_KEY(StrafeRight,    KEY_STRAFERIGHT);
CONTROL_KEY(Run,            KEY_RUN);
CONTROL_KEY(Jump,           KEY_JUMP);
CONTROL_KEY(LookUp,         KEY_LOOKUP);
CONTROL_KEY(LookDown,       KEY_LOOKDOWN);

//
// FCmd_Bind
//

static void FCmd_Bind(void)
{
    int argc;
    int key;
    int i;
    char cmd[1024];

    argc = Cmd_GetArgc();

    if(argc < 3)
    {
        Com_Printf("bind <key> <command>\n");
        return;
    }

    if(!(key = Key_GetKeyCode(Cmd_GetArgv(1))))
    {
        Com_Warning("\"%s\" isn't a valid key\n", Cmd_GetArgv(1));
        return;
    }

    cmd[0] = 0;
    for(i = 2; i < argc; i++)
    {
        strcat(cmd, Cmd_GetArgv(i));
        if(i != (argc - 1))
        {
            strcat(cmd, " ");
        }
    }

    Key_BindCmd(key, cmd);
}

//
// FCmd_UnBind
//

static void FCmd_UnBind(void)
{
}

//
// FCmd_ListBinds
//

static void FCmd_ListBinds(void)
{
    keycmd_t *keycmd;
    cmdlist_t *cmd;
    int i;

    Com_Printf("\n");

    for(i = 0; i < MAX_KEYS; i++)
    {
        keycmd = &keycmds[i];

        for(cmd = keycmd->cmds; cmd; cmd = cmd->next)
        {
            char buff[32];

            Key_GetName(buff, i);
            strlwr(buff);

            Com_CPrintf(COLOR_GREEN, "%s : \"%s\"\n", buff, cmd->command);
        }
    }
}

//
// Key_Init
//

void Key_Init(void)
{
    int c;

    for(c = 0; c < MAX_KEYS; c++)
    {
        keycode[0][c] = c;
        keycode[1][c] = c;
        keydown[c] = false;
        keycmds[c].cmds = NULL;
    }

    keycode[1]['1'] = '!';
    keycode[1]['2'] = '@';
    keycode[1]['3'] = '#';
    keycode[1]['4'] = '$';
    keycode[1]['5'] = '%';
    keycode[1]['6'] = '^';
    keycode[1]['7'] = '&';
    keycode[1]['8'] = '*';
    keycode[1]['9'] = '(';
    keycode[1]['0'] = ')';
    keycode[1]['-'] = '_';
    keycode[1]['='] = '+';
    keycode[1]['['] = '{';
    keycode[1][']'] = '}';
    keycode[1]['\\'] = '|';
    keycode[1][';'] = ':';
    keycode[1]['\''] = '"';
    keycode[1][','] = '<';
    keycode[1]['.'] = '>';
    keycode[1]['/'] = '?';
    keycode[1]['`'] = '~';
    
    for(c = 'a'; c <= 'z'; c++)
    {
        keycode[1][c] = toupper(c);
    }

    Cmd_AddCommand("bind", FCmd_Bind);
    Cmd_AddCommand("unbind", FCmd_UnBind);
    Cmd_AddCommand("listbinds", FCmd_ListBinds);
    Cmd_AddCommand("+attack", FCmd_AttackDown);
    Cmd_AddCommand("-attack", FCmd_AttackUp);
    Cmd_AddCommand("+forward", FCmd_ForwardDown);
    Cmd_AddCommand("-forward", FCmd_ForwardUp);
    Cmd_AddCommand("+back", FCmd_BackDown);
    Cmd_AddCommand("-back", FCmd_BackUp);
    Cmd_AddCommand("+left", FCmd_LeftDown);
    Cmd_AddCommand("-left", FCmd_LeftUp);
    Cmd_AddCommand("+right", FCmd_RightDown);
    Cmd_AddCommand("-right", FCmd_RightUp);
    Cmd_AddCommand("+strafeleft", FCmd_StrafeLeftDown);
    Cmd_AddCommand("-strafeleft", FCmd_StrafeLeftUp);
    Cmd_AddCommand("+straferight", FCmd_StrafeRightDown);
    Cmd_AddCommand("-straferight", FCmd_StrafeRightUp);
    Cmd_AddCommand("+run", FCmd_RunDown);
    Cmd_AddCommand("-run", FCmd_RunUp);
    Cmd_AddCommand("+jump", FCmd_JumpDown);
    Cmd_AddCommand("-jump", FCmd_JumpUp);
    Cmd_AddCommand("+lookup", FCmd_LookUpDown);
    Cmd_AddCommand("-lookup", FCmd_LookUpUp);
    Cmd_AddCommand("+lookdown", FCmd_LookDownDown);
    Cmd_AddCommand("-lookdown", FCmd_LookDownUp);
}
