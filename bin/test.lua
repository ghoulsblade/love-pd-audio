-- test with raw lua

print("01")
require("lovepdaudio")
print("02")
lovepdaudio.helloworld() 
print("03")
-- local filepath = "test.pd"
local filepath = "pdnes.pd"
-- local filepath = "MAIN_musicmachine.pd"
--~ lovepdaudio.test02(filepath) 

function libpdhook (event,...) print("libpdhook",event,...) end

print("04")
gPDPlayer = lovepdaudio.CreatePureDataPlayer(filepath)
print("05")

local i = 1
while (true) do lovepdaudio.PureDataPlayer_Update(gPDPlayer) if (i < 10) then print("updated.") i = i + 1 end end
	
print("end.")
