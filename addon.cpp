#include <string>
#include <string_view>
#include <fstream>
#include <sstream>
#include <random>
#include <streambuf>

#include "interface.h"
#include "engine/iserverplugin.h"
#include "tier0/icommandline.h"
#include "icvar.h"
#include "tier1/convar.h"
#include "tier0/memdbgon.h"

class CEmptyServerPlugin: public IServerPluginCallbacks
{
public:
	virtual bool			Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory );
	virtual void			Unload( void ) {}
	virtual void			Pause( void ) {}
	virtual void			UnPause( void ) {}
	virtual const char     *GetPluginDescription( void ) { return "random_map"; }
	virtual void			LevelInit( char const *pMapName ) {}
	virtual void			ServerActivate( edict_t *pEdictList, int edictCount, int clientMax ) {}
	virtual void			GameFrame( bool simulating ) {}
	virtual void			LevelShutdown( void ) {}
	virtual void			ClientActive( edict_t *pEntity ) {}
	virtual void			ClientDisconnect( edict_t *pEntity ) {}
	virtual void			ClientPutInServer( edict_t *pEntity, char const *playername ) {}
	virtual void			SetCommandClient( int index ) {}
	virtual void			ClientSettingsChanged( edict_t *pEdict ) {}
	virtual PLUGIN_RESULT	ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen ) { return PLUGIN_CONTINUE; }
	virtual PLUGIN_RESULT	ClientCommand( edict_t *pEntity, const CCommand &args )  { return PLUGIN_CONTINUE; }
	virtual PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )  { return PLUGIN_CONTINUE; }
	virtual void			OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue ) {}
	virtual void			OnEdictAllocated( edict_t *edict ) {}
	virtual void			OnEdictFreed( const edict_t *edict  ) {}
};

#define INTERFACEVERSION_ISERVERPLUGINCALLBACKS_VERSION_3	"ISERVERPLUGINCALLBACKS003"

#define TIER1_BUGGED

CEmptyServerPlugin g_EmtpyServerPlugin;
#ifndef TIER1_BUGGED
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CEmptyServerPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS_VERSION_3, g_EmtpyServerPlugin);
#else
extern "C" __attribute__((visibility ("default"))) void *CreateInterface(const char *name, int *ret)
{
	if(strstr(name, "ISERVERPLUGINCALLBACKS") == name) {
		if(ret) {
			*ret = 1;
		}
		return &g_EmtpyServerPlugin;
	}
	
	if(ret) {
		*ret = 0;
	}
	return nullptr;
}
#endif

class CCvar
{
public:
	static void Unlock(ConCommandBase *pCmd)
	{
		pCmd->m_nFlags &= ~(FCVAR_DEVELOPMENTONLY|FCVAR_HIDDEN|FCVAR_NOT_CONNECTED|FCVAR_SPONLY);
		pCmd->m_nFlags |= (FCVAR_SERVER_CAN_EXECUTE|FCVAR_CLIENTCMD_CAN_EXECUTE|FCVAR_REPLICATED);
		if(!pCmd->IsCommand()) {
			ConVar *pCvar = (ConVar *)pCmd;
			pCvar->m_bHasMin = false;
			pCvar->m_bHasMax = false;
		}
	}
	
	static ConCommandBase *GetNext(ConCommandBase *pCmd)
	{
		return pCmd->m_pNext;
	}
};

ICvar *g_pCVar = nullptr;

void RemoveOption(std::string &str, std::string_view find)
{
	size_t it = str.find(find);
	if(it != std::string::npos) {
		str.erase(it, it+find.length()+1);
		while(str[it] != ' ') {
			str.erase(it);
		}
		if(str[it] == ' ') {
			str.erase(it);
		} else if(str[it] == '\0') {
			str.erase(it-1);
		}
	}
}

void RemoveSwitch(std::string &str, std::string_view find)
{
	size_t it = str.find(find);
	if(it != std::string::npos) {
		str.erase(it, it+find.length());
		if(str[it] == ' ') {
			str.erase(it);
		} else if(str[it] == '\0') {
			str.erase(it-1);
		}
	}
}

bool CEmptyServerPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
	g_pCVar = (ICvar *)interfaceFactory(CVAR_INTERFACE_VERSION, nullptr);
	
	ConCommandBase *pCVar = g_pCVar->GetCommands();
	while(pCVar) {
		CCvar::Unlock(pCVar);
	#ifndef TIER1_BUGGED
		pCVar = pCVar->GetNext();
	#else
		pCVar = CCvar::GetNext(pCVar);
	#endif
	}
	
	ICommandLine *pCommandLine = CommandLine();
	
	std::string old_cmdline = pCommandLine->GetCmdLine();
	std::string new_cmdline = old_cmdline;
	
	RemoveOption(new_cmdline, "-map");
	RemoveOption(new_cmdline, "+map");
	
	RemoveSwitch(new_cmdline, "+randommap");
	RemoveSwitch(new_cmdline, "-randommap");
	
	std::ifstream infile = std::ifstream("tf/addons/random_map.txt");
	if (infile.is_open()) {
		std::stringstream line;
		line << infile.rdbuf();
		new_cmdline += ' ';
		new_cmdline += line.str();
	}
	
	ConMsg("[RANDOM_MAP]\n\tOldCmdLine: %s\n\tNewCmdLine: %s\n", old_cmdline.c_str(), new_cmdline.c_str());
	
	pCommandLine->CreateCmdLine( new_cmdline.c_str() );
	
	return true;
}
