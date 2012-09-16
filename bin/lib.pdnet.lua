-- lua lib for sending to puredata netreceive, does not require the lovepdaudio dlls
-- see also kalimba : http://blog.tridek.com/post/29834155461/using-libpd-with-unity3d-on-mobile-devices-part-1
-- example :   	PDNet:Init()  ...   PDNet:SendBangToReceiver("test2")
local socket = require ("socket")

PDNet = {}

function PDNet:Error (...) print("PDNet error",...) end

function PDNet:Init(host,port)
	self.client = nil
	self.stream = nil
	self.host = host or "127.0.0.1"
	self.port = port or 32000
	self:setup()
end


	
function PDNet:setup()
	if (self.socket == nil or (not self.bConnected)) then
		self.bConnected = nil
		if (self.socket) then self.socket:close() end
		self.socket = socket.tcp() if (not self.socket) then return self:Error("failed to open socket") end 
		local res,msg = self.socket:connect(self.host,self.port)
		if (not res) then return self:Error(msg) end
		self.bConnected = true
	end
	return true
end
	
local function ASCIIEncode (txt) return tostring(txt) end -- TODO? ASCIIEncoding http://msdn.microsoft.com/en-us/library/system.text.asciiencoding.aspx
local TRIM_WHITESPACE = "[ \t\n\r]" -- White-space characters are defined by the Unicode standard. http://en.wikipedia.org/wiki/Whitespace_character#Unicode
local function TrimStart	(txt,pattern) txt = string.gsub(txt,"^"..(pattern or TRIM_WHITESPACE).."+","") return txt end
local function TrimEnd		(txt,pattern) txt = string.gsub(txt,(pattern or TRIM_WHITESPACE).."+$","") return txt end
local function Trim			(txt) return TrimStart(TrimEnd(txt)) end -- Trim: http://msdn.microsoft.com/en-us/library/t97s7bs3.aspx  

function PDNet:sendPdMessage	(message)
	if (self.bConnected) then
		local data = ASCIIEncode(Trim(TrimEnd(Trim(message),";"))..";")
		self.socket:send(data)
	else
		self:Error("could not send message " .. message .. " to " .. self.host .. ":" .. self.port)
	end
end

function PDNet:constructAndSendMessagesToSendMessage	(message)
	self:setup()
	self:sendPdMessage("set;")
	self:sendPdMessage("addsemi;")
	self:sendPdMessage("add " .. message)
	self:sendPdMessage("bang;")
end

function PDNet:SendBangToReceiver	(receiverName)
	self:constructAndSendMessagesToSendMessage(receiverName .. " bang")
end
	
function PDNet:SendFloat	(val, receiverName)
	self:constructAndSendMessagesToSendMessage(receiverName .. " " .. tostring(val))
end
	
function PDNet:SendSymbol	(symbol, receiverName)
	self:constructAndSendMessagesToSendMessage(receiverName .. " " .. symbol)
end
