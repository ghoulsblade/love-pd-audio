-- test with raw lua

print("01")
require("lovepdaudio")
print("02")
lovepdaudio.helloworld() 
print("03")
local filepath = "test.pd"
local filepath = "pdnes.pd"
--~ lovepdaudio.test02(filepath) 

function libpdhook (event,...) print("libpdhook",event,...) end

gPDPlayer = lovepdaudio.CreatePureDataPlayer(filepath)
while (true) do lovepdaudio.PureDataPlayer_Update(gPDPlayer) end
	
print("04")
