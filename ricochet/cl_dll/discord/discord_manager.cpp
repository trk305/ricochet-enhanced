// ================================================== \\
// Discord RPC implemenatation
// Based on code from the Valve Dev Community wiki
// 
// Jay - 2022
// ================================================== \\
// ================================================== \\

#include "hud.h"
#include "discord_manager.h"
#include <string.h>
#include "discord\discord_rpc.h"
#include <time.h>

static DiscordRichPresence discordPresence;
extern cl_enginefunc_t gEngfuncs;

// Blank handlers
static void HandleDiscordReady(const DiscordUser* connectedUser) {}
static void HandleDiscordDisconnected(int errcode, const char* message) {}
static void HandleDiscordError(int errcode, const char* message) {}
static void HandleDiscordJoin(const char* secret) {}
static void HandleDiscordSpectate(const char* secret) {}
static void HandleDiscordJoinRequest(const DiscordUser* request) {}

// Version info
#define RICOCHET_VERSION "VERSION 3 DEV"

// Default logo
const char* defaultLogo = "logo";

// Cache for reducing updates
static char lastMapName[64] = "";
static int lastPlayerCount = -1;
static int lastMaxPlayers = -1;
static char lastServerName[128] = "";

// Helper: Extract clean map name from path
static void ExtractMapName(const char* mapPath, char* outBuffer, size_t bufferSize)
{
	if (!mapPath || !outBuffer || bufferSize == 0)
		return;

	const char* lastSlash = strrchr(mapPath, '/');
	const char* lastBackslash = strrchr(mapPath, '\\');
	const char* mapName = lastSlash > lastBackslash ? lastSlash : lastBackslash;

	if (!mapName)
		mapName = mapPath;
	else
		mapName++;

	strncpy(outBuffer, mapName, bufferSize - 1);
	outBuffer[bufferSize - 1] = '\0';

	// Strip .bsp extension
	char* bspExt = strstr(outBuffer, ".bsp");
	if (bspExt)
		*bspExt = '\0';
}

// Helper: Count active players from scoreboard
static int GetPlayerCount(int* outMaxPlayers)
{
	int playerCount = 0;
	int maxPlayers = gEngfuncs.GetMaxClients();

	if (outMaxPlayers)
		*outMaxPlayers = maxPlayers;

	// Count all connected players (including bots)
	for (int i = 1; i <= maxPlayers; i++)
	{
		hud_player_info_t playerInfo;
		gEngfuncs.pfnGetPlayerInfo(i, &playerInfo);

		if (playerInfo.name != NULL && playerInfo.name[0] != '\0')
		{
			playerCount++;
		}
	}

	return playerCount;
}

void DiscordMan_Startup(void)
{
	int64_t startTime = time(0);
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	handlers.ready = HandleDiscordReady;
	handlers.disconnected = HandleDiscordDisconnected;
	handlers.errored = HandleDiscordError;
	handlers.joinGame = HandleDiscordJoin;
	handlers.spectateGame = HandleDiscordSpectate;
	handlers.joinRequest = HandleDiscordJoinRequest;

	Discord_Initialize("1296747711541936129", &handlers, 1, 0);

	memset(&discordPresence, 0, sizeof(discordPresence));
	discordPresence.startTimestamp = startTime;
	discordPresence.details = RICOCHET_VERSION;
	discordPresence.state = "Main Menu - Looking for Ricodudes";
	discordPresence.largeImageKey = defaultLogo;
	discordPresence.largeImageText = "Download Ricochet Enhanced Now!";

	Discord_UpdatePresence(&discordPresence);

	gEngfuncs.Con_Printf("[Discord RPC] Initialized successfully\n");
}

void DiscordMan_Update(void)
{
	Discord_RunCallbacks();

	char rawMapName[64];
	char stateText[128];
	char serverName[128];
	int currentPlayers = 0;
	int maxPlayers = 0;
	int isSpectator = 0;

	// Get current map (raw name, no formatting)
	ExtractMapName(gEngfuncs.pfnGetLevelName(), rawMapName, sizeof(rawMapName));

	// Check if we're in main menu (no map loaded)
	if (rawMapName[0] == '\0')
	{
		// Only update if we were previously in a map
		if (lastMapName[0] != '\0')
		{
			discordPresence.details = RICOCHET_VERSION;
			discordPresence.state = "Main Menu - Looking for Ricodudes";
			discordPresence.largeImageKey = defaultLogo;
			discordPresence.largeImageText = "Download Ricochet Enhanced Now!";
			discordPresence.partySize = 0;
			discordPresence.partyMax = 0;

			Discord_UpdatePresence(&discordPresence);

			// Clear cache
			lastMapName[0] = '\0';
			lastServerName[0] = '\0';
			lastPlayerCount = -1;
			lastMaxPlayers = -1;
		}
		return;
	}

	// Get server hostname
	const char* hostname = gEngfuncs.pfnGetCvarString("hostname");
	if (hostname && hostname[0] != '\0')
	{
		strncpy(serverName, hostname, sizeof(serverName) - 1);
		serverName[sizeof(serverName) - 1] = '\0';
	}
	else
	{
		strncpy(serverName, "Local Server", sizeof(serverName) - 1);
	}

	// Check if spectating/observing
	cl_entity_t* localPlayer = gEngfuncs.GetLocalPlayer();

	// Spectators have MOVETYPE_NOCLIP (8) set in spectator.cpp
#define MOVETYPE_NOCLIP 8

	if (localPlayer->curstate.movetype == MOVETYPE_NOCLIP)
	{
		isSpectator = 1;
	}

	// Get player count (including bots)
	currentPlayers = GetPlayerCount(&maxPlayers);

	// Build state text with server name, spectator status, and player count
	if (isSpectator)
	{
		if (currentPlayers > 0)
		{
			snprintf(stateText, sizeof(stateText), "Spectating on %s | %s | %d/%d Players",
				serverName, rawMapName, currentPlayers, maxPlayers);
		}
		else
		{
			snprintf(stateText, sizeof(stateText), "Spectating on %s | %s",
				serverName, rawMapName);
		}
	}
	else
	{
		if (currentPlayers > 0)
		{
			snprintf(stateText, sizeof(stateText), "Playing on %s | %s | %d/%d Players",
				serverName, rawMapName, currentPlayers, maxPlayers);
		}
		else
		{
			snprintf(stateText, sizeof(stateText), "Playing on %s | %s",
				serverName, rawMapName);
		}
	}

	// Check if we need to update (avoid spam)
	int needsUpdate = 0;
	if (strcmp(lastMapName, rawMapName) != 0 ||
		strcmp(lastServerName, serverName) != 0 ||
		lastPlayerCount != currentPlayers ||
		lastMaxPlayers != maxPlayers)
	{
		needsUpdate = 1;
		strncpy(lastMapName, rawMapName, sizeof(lastMapName) - 1);
		strncpy(lastServerName, serverName, sizeof(lastServerName) - 1);
		lastPlayerCount = currentPlayers;
		lastMaxPlayers = maxPlayers;
	}

	if (needsUpdate)
	{
		// Update presence
		discordPresence.details = isSpectator ? "Spectating [" RICOCHET_VERSION "]" : "Fighting Ricodudes [" RICOCHET_VERSION "]";
		discordPresence.state = stateText;
		discordPresence.largeImageKey = defaultLogo;
		discordPresence.largeImageText = "Download Ricochet Enhanced Now!";

		// Set party size for player count display
		if (currentPlayers > 0)
		{
			discordPresence.partySize = currentPlayers;
			discordPresence.partyMax = maxPlayers;
		}
		else
		{
			discordPresence.partySize = 0;
			discordPresence.partyMax = 0;
		}

		Discord_UpdatePresence(&discordPresence);
	}
}

void DiscordMan_Kill(void)
{
	Discord_Shutdown();
	gEngfuncs.Con_Printf("[Discord RPC] Shutdown complete\n");
}