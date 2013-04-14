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
// DESCRIPTION: Main Client Code
//
//-----------------------------------------------------------------------------

#include "enet/enet.h"
#include "common.h"
#include "packet.h"
#include "client.h"
#include "kernel.h"
#include "render.h"
#include "menu.h"
#include "gl.h"
#include "game.h"
#include "mathlib.h"
#include "js.h"
#include "sound.h"
#include "fx.h"

client_t client;

static kbool bDebugTime;

CVAR(cl_name, player);
CVAR(cl_fov, 74);
CVAR(cl_autorun, 0);
CVAR(cl_paused, 0);
CVAR(cl_msensitivityx, 5);
CVAR(cl_msensitivityy, 5);
CVAR(cl_macceleration, 0);
CVAR(cl_mlook, 0);
CVAR(cl_mlookinvert, 0);
CVAR(cl_port, 58304);

CVAR_CMD(cl_maxfps, 60)
{
    if(cvar->value <= 0)
        cvar->value = 1;
}

CVAR_EXTERNAL(developer);

//
// CL_DestroyClient
//

void CL_DestroyClient(void)
{
    if(client.host)
    {
        if(client.peer != NULL)
        {
            enet_peer_disconnect(client.peer, 0);
            enet_peer_reset(client.peer);
        }

        enet_host_destroy(client.host);
        client.host = NULL;
        client.state = CL_STATE_DISCONNECTED;
    }
}

//
// CL_Shutdown
//

void CL_Shutdown(void)
{
    CL_DestroyClient();
}

//
// CL_ProcessServerPackets
//

void CL_ProcessServerPackets(ENetPacket *packet, ENetEvent *cev)
{
    int type = 0;

    Packet_Read8(packet, &type);

    switch(type)
    {
    case sp_ping:
        Com_Printf("Recieved acknowledgement from server\n");
        break;

    case sp_clientinfo:
        Packet_Read8(packet, &client.client_id);
        client.player->info.id = client.client_id;
        client.state = CL_STATE_READY;
        Com_DPrintf("CL_ReadClientInfo: ID is %i\n", client.client_id);
        break;

    default:
        Com_Warning("Recieved unknown packet type: %i\n", type);
        break;
    }

    enet_packet_destroy(cev->packet);
}

//
// CL_Connect
//

void CL_Connect(const char *address)
{
    ENetAddress addr;
    char ip[32];

    enet_address_set_host(&addr, address);
    addr.port = (int)cl_port.value;
    client.peer = enet_host_connect(client.host, &addr, 2, 0);
    client.player->info.peer = client.peer;
    enet_address_get_host_ip(&addr, ip, 32);
    Com_Printf("Connecting to %s:%u...\n", ip, addr.port);

    if(client.peer == NULL)
    {
        Com_Warning("No available peers for initiating an ENet connection.\n");
        return;
    }

    client.state = CL_STATE_CONNECTING;
}

//
// CL_CheckHostMsg
//

static void CL_CheckHostMsg(void)
{
    ENetEvent cev;

    while(enet_host_service(client.host, &cev, 0) > 0)
    {
        switch(cev.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            client.state = CL_STATE_CONNECTED;
            P_ResetNetSeq(&client.player->info);
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            client.state = CL_STATE_DISCONNECTED;
            break;

        case ENET_EVENT_TYPE_RECEIVE:
            CL_ProcessServerPackets(cev.packet, &cev);
            break;
        }

        client.netEvent = cev;
        //J_RunObjectEvent(JS_EV_CLIENT, "netUpdate");
    }
}

//
// CL_DrawDebug
//

static void CL_DrawDebug(void)
{
    if(bDebugTime)
    {
        Draw_Text(32, 16,  COLOR_GREEN, 1, "-------------------");
        Draw_Text(32, 32,  COLOR_GREEN, 1, "   client debug");
        Draw_Text(32, 48,  COLOR_GREEN, 1, "-------------------");
        Draw_Text(32, 64,  COLOR_GREEN, 1, "FPS: %i", (int)(1000.0f / (client.runtime * 1000.0f)));
        Draw_Text(32, 80,  COLOR_GREEN, 1, "runtime: %f", client.runtime);
        Draw_Text(32, 96,  COLOR_GREEN, 1, "time: %i", client.time);
        Draw_Text(32, 112,  COLOR_GREEN, 1, "tics: %i", client.tics);
        Draw_Text(32, 128, COLOR_GREEN, 1, "max msecs: %f",
            (1000.0f / cl_maxfps.value) / 1000.0f);
        /*Draw_Text(32, 144, COLOR_GREEN, 1, "server time: %i",
            client.st.tics - (client.st.time/100));
        Draw_Text(32, 160, COLOR_GREEN, 1, "latency: %i",
            client.time - client.latency[client.ns.acks & (NETBACKUPS-1)]);*/
        Draw_Text(32, 144, COLOR_WHITE, 1, Con_GetLastBuffer());
        return;
    }

    if(developer.value)
        Draw_Text(32, 32, COLOR_WHITE, 1, Con_GetLastBuffer());
}

//
// CL_Run
//

void CL_Run(int msec)
{
    static int curtime = 0;

    curtime += msec;

    if(curtime < (1000 / (int)cl_maxfps.value))
        return;

    client.runtime = (float)curtime / 1000.0f;
    client.time += curtime;

    curtime = 0;

    CL_CheckHostMsg();

    IN_PollInput();

    CL_ProcessEvents();

    //J_RunObjectEvent(JS_EV_CLIENT, "tick");

    P_BuildCommands();

    P_LocalPlayerTick();

    Menu_Ticker();

    Con_Ticker();

    FX_Ticker();

    //TEMP
    G_ClientThink();

    R_DrawFrame();

    CL_DrawDebug();

    Menu_Drawer();

    Con_Drawer();

    R_FinishFrame();

    Snd_UpdateListener();

    client.tics++;
}

//
// CL_MessageServer
//

void CL_MessageServer(char *string)
{
    ENetPacket *packet;

    if(!(packet = Packet_New()))
        return;

    Packet_Write8(packet, cp_msgserver);
    Packet_WriteString(packet, string);
    Packet_Send(packet, client.peer);
}

//
// FCmd_ShowClientTime
//

static void FCmd_ShowClientTime(void)
{
    bDebugTime ^= 1;
}

//
// FCmd_Ping
//

static void FCmd_Ping(void)
{
    ENetPacket *packet;

    if(!(packet = Packet_New()))
        return;

    Packet_Write8(packet, cp_ping);
    Packet_Send(packet, client.peer);
}

//
// FCmd_Say
//

static void FCmd_Say(void)
{
    ENetPacket *packet;

    if(Cmd_GetArgc() < 2)
        return;

    if(!(packet = Packet_New()))
        return;

    Packet_Write8(packet, cp_say);
    Packet_WriteString(packet, Cmd_GetArgv(1));
    Packet_Send(packet, client.peer);
}

//
// FCmd_MsgServer
//

static void FCmd_MsgServer(void)
{
    if(Cmd_GetArgc() < 2)
        return;

    CL_MessageServer(Cmd_GetArgv(1));
}

//
// CL_Init
//

void CL_Init(void)
{
    memset(&client, 0, sizeof(client_t));

    CL_DestroyClient();

    client.host = enet_host_create(NULL, 1, 2, 0, 0);

    if(!client.host)
    {
        Com_Error("CL_Init: failed to create client");
        return;
    }

    bDebugTime = false;

    client.client_id = -1;
    client.time = 0;
    client.tics = 0;
    client.peer = NULL;
    client.state = CL_STATE_UNINITIALIZED;
    client.local = (Com_CheckParam("-client") == 0);
    client.playerActor = NULL;
    client.player = &localPlayer;

    Cvar_Register(&cl_name);
    Cvar_Register(&cl_fov);
    Cvar_Register(&cl_autorun);
    Cvar_Register(&cl_maxfps);
    Cvar_Register(&cl_paused);
    Cvar_Register(&cl_msensitivityx);
    Cvar_Register(&cl_msensitivityy);
    Cvar_Register(&cl_macceleration);
    Cvar_Register(&cl_mlook);
    Cvar_Register(&cl_mlookinvert);
    Cvar_Register(&cl_port);

    Cmd_AddCommand("debugclienttime", FCmd_ShowClientTime);
    Cmd_AddCommand("ping", FCmd_Ping);
    Cmd_AddCommand("say", FCmd_Say);
    Cmd_AddCommand("msgserver", FCmd_MsgServer);
}
