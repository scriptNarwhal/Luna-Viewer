--[[
	Luna Lua Event Handler
		by N3X15
	
	Copyright (C) 2008-2009 Luna Contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	 
	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 
	$Id$
	
	*** DO NOT MODIFY OR YOU WILL BREAK SHIT. ***
]]--

--[[ Coded events:	All arguments are received as strings. Types below indicate string format. 
			Eg: With 'integer MyVar' MyVar will be a string such as "2", which is 
			    safe to handle as an integer. Lua will even implicitly convert it in 
			    this case. For instance statements like if(MyVar==2) are valid.

			LLUUIDs and Vectors are also automatically converted to native types.
			    
	 EventName:		( args ):
	 
	OnLuaInit 		( )
	OnAgentInit 		( string AvName,	integer GodMode	)
	EmeraldPhantomOn	( integer Phantom )
	OnObjectCreated 	( string ObjectID, 	string PrimCode )
	OnAttach 		( string ObjectID, 	string AvName	)
	OnAvatarLoaded		( string UUID, string FullName, string RegionID )

	NOTE:  Message is delimited by "|".
	OnBridgeMessage 	(integer Channel, string from_name, string source_id, string owner_id, string message)
	OnBridgeReady		(integer Channel)
	OnBridgeFailed		( )
	OnBridgeWorking		(integer Channel)

	OnAttachedSound		(LLUUID object_id, LLUUID audio_uuid, LLUUID owner_id, float gain, integer flags)
	OnAttachedParticles	(LLUUID object_id, LLUUID owner_id, LLUUID image_id, string ParticleSystemInfo)

	OnRegionChanged		(string Name, string ip)
	
	OnChatWhisper		(UUID from_id, UUID owner_id, string mesg)
	OnChatSay		(UUID from_id, UUID owner_id, string mesg)
	OnChatShout		(UUID from_id, UUID owner_id, string mesg)
	OnOwnerSay		(UUID from_id, UUID owner_id, string mesg)
	OnChatDebug		(UUID from_id, UUID owner_id, string mesg)

	OnSoundTriggered	(sound_id, owner_id, gain, object_id, parent_id)

// This is going to change next revision
	LUA_CALL("OnSetText") << temp_string << getID() << LUA_END;
	
]]--

gEvents={};
gEventDescs={};

function SetHook(EventName,Func)
	if(gEvents[EventName]==nil) then RegisterHook(EventName,"Autoregistered by SetHook()") end
	table.insert(gEvents[EventName],Func);
end

-- Should only be called from Luna's C++ code, or from a package that initialized the event called.
function CallHook(EventName,...)
	if(gEvents[EventName]==nil) then return 0 end -- No hooks to call, so exit.
	val=0
	for _,hookedfunc in pairs(gEvents[EventName]) do 
		hookedfunc(...) 
		val=val + 1 
	end
	return val
end

function RegisterHook(EventName,Desc)
	if(gEvents[EventName]==nil) then
		gEvents[EventName]={};
		gEventDescs[EventName]=Desc
		return
	end
	error(EventName.." has already been registered!")
end

function DumpAllHooks()
	print "Registered Hooks:"
	for name,_ in pairs(gEvents) do
		hookdesc=""
		if(gEventDescs[name]==nil) then 
			hookdesc=gEventDescs[name] 
		else 
			hookdesc="[EVENT LACKS A DESCRIPTION!]" 
		end
		print(name..": "..gEventDescs[name])
	end
end

function DumpAllHooks2Wiki()
	local txt = "Registered hooks in Luna ".._SLUA_VERSION..":"
	local h=gEvents
	table.sort(h)
	for name,_ in pairs(h) do
		hookdesc=""
		if(gEventDescs[name]==nil) then 
			hookdesc=gEventDescs[name] 
		else 
			hookdesc="''EVENT LACKS A DESCRIPTION!''" 
		end
		txt=txt.."\n;[["..name.."]]\n:"..gEventDescs[name].."\n"
	end
	return txt
end

