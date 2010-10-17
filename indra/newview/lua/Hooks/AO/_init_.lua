--[[
	Luna Lua AO Plugin
		by N3X15

	Copyright (C) 2008-2010 Luna Contributors

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
]]--

AO={}

-- For opening notecards
AO.Parsers={}

-- State={Replacements},
AO.AnimationOverrides={}

-- Current Animations
AO.CurrentAnimation=UUID_nil
AO.CurrentState=""
AO.Disabled=false
AO.Debug=false -- TURN ON DEBUG MODE (/lua AO.Debug=true)

--OnAnimStart(avid,id,time_offset) For AOs.
function AO:OnAnimStart(av_id,id,time_offset)
	-- Ensure that we're only receiving AnimationStarts that come from our own avatar.
	if tostring(av_id) == tostring(getMyID()) then return end
	
	-- Also make sure that the AO is actually enabled.
	if self.Disabled==true then return end
	
	-- If nothing's loaded, see if cached.lua exists
	-- Also set our cache folder
	if self.AnimationOverrides == {} then
		lfs.mkdir(getDataDir().."/AO/")
		self.CacheFile=getDataDir().."/AO/cached.lua"
		self:Load()
	end
	
	-- If it's a known viewer-generated animation, ignore it.
	if GeneratedAnims[id] ~= nil then
		return
	end
	
	-- Just in case.
	id=tostring(id)
	
	-- this is all for debugging.
	name=AnimationNames[id]
	if (name==nil) then
		name = "???"
	end
	
	st8=AnimationStates[id]
	if(st8==nil) then
		st8="???"
	end
	self:DebugInfo("Animation playing: "..id.." ("..name.." @ state "..state..")")
	
	-- If the animation changes state of the avatar, 
	if AnimationStates[id] ~= nil and self.CurrentAnimation ~= id then
		state=AnimationStates[id]
		
		-- Debugging shit
		if self.CurrentState ~= state then
			self:DebugInfo("State changed to "..state)
			self.CurrentState=state
		end
		
		-- Check if the state has overrides assigned.  If not, abort and let the defaults play.
		if (self.AnimationOverrides==nil or self.AnimationOverrides[id]==nil) then return end
		
		-- Fetch the number and collection of overrides.
		numreplacements=table.getn(self.AnimationOverrides[id])
		replacements=self.AnimationOverrides[id]
		
		-- If we're already playing an override animation, stop it.
		if self.CurrentAnimation ~= UUID_nil then
			self:DebugInfo("Stopping motion "..self.CurrentAnimation)
			stopAnimation(getMyID(),self.CurrentAnimation)
		end
		
		-- If there are overrides assigned for this state...
		if numreplacements > 0 then
		
			-- Get a random one
			self.CurrentAnimation=replacements[math.random(1,numreplacements)]
			self:DebugInfo("Playing motion "..self.CurrentAnimation)
			-- Stop the animation being overridden
			stopAnimation(getMyID(),id)
			-- And play the override.
			startAnimation(getMyID(), self.CurrentAnimation)
		end
	end
end

function AO:ClearOverrides()
	self.AnimationOverrides={}
end

function AO:AddOverride(state,anim)
	-- If there's no category in the overrides table for the desired state, add it.
	if self.AnimationOverrides[state] == nil then
		self.AnimationOverrides[state]={}
	end
	
	-- If we're not looking at a UUID, it's probably a name. (God help us if they saved it as a UUID)
	if not UUID_validate(anim) then
		-- Get the UUID associated with this name, if possible.
		anim_name=anim
		anim=getInventoryItemUUID(anim,AssetType.ANIMATION)
		-- If not possible, ABORT ABORT
		if anim == UUID_null then return end
		
		-- Debugging
		print("[AO] "..anim_name.." -> "..anim)
	end
	-- Add animation override to the overrides table in the desired State category.
	i=table.getn(self.AnimationOverrides[state])+1
	self.AnimationOverrides[state][i]=anim
	
	-- SAVE.
	self:Save()
end

function AO:Save()
	if getMyID() ~= UUID_null and self.CacheFile ~= nil then
		table.save(self.AnimationOverrides,self.CacheFile)
	end
end

function AO:Load()
	if getMyID() ~= UUID_null and self.CacheFile ~= nil then
		if not exists(self.CacheFile) then return end
		tmptable,err = table.load(self.AnimationOverrides,self.CacheFile)
		if err~=nil then
			error("[AO] Failed to load cache: "..err)
		else
			self.AnimationOverrides=tmptable
		end
	end
end

function AO:DebugInfo(msg)
	if self.Debug==true then
		print("[AO]: "..msg)
	end
end

-- Detect what kind of notecard we're dealing with.
function AO:DetectNotecardType(data)
end

------------------------------------------------------------------------------------------------------------------
--                                             EVENT HANDLING                                                   --
------------------------------------------------------------------------------------------------------------------
local function AO_OnAONotecard(id)
	-- Download the notecard and decode it.
	print ("[AO] Downloading notecard.")
	AO.NotecardRequestKey=requestInventoryAsset(id,UUID_null)
end

local function AO_OnAssetFailed(transfer_key)
	if AO.NotecardRequestKey == transfer_key then
		AO.NotecardRequestKey=nil
		error("Failed to download the notecard.  Please try again later.  You can also try from a different sim.")
	end
end

local function AO_OnAssetDownloaded(transfer_key,typ,data)
	if AO.NotecardRequestKey == transfer_key then
		AO.NotecardRequestKey=nil
		print("[AO] Notecard successfully downloaded! Attempting to parse...")
		AO:DetectNotecardType(data)
	end
end

local function AO_OnAnimStart(av_id,id,time_offset)
	AO:OnAnimStart(av_id,id,time_offset)
end

SetHook("OnAnimStart",AO_OnAnimStart)
SetHook("OnAONotecard",AO_OnAONotecard)
SetHook("OnAssetDownloaded",AO_OnAssetDownloaded)
SetHook("OnAssetFailed",AO_OnAssetFailed)