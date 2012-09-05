-- test with love2d

print("01")
require("lovepdaudio")
print("02")
lovepdaudio.helloworld() 
print("03")

gDoPDUpdate = true
function love.load () 
end
function love.update () 
	gPDPlayer = gPDPlayer or lovepdaudio.CreatePureDataPlayer("pdnes.pd")
	if (gDoPDUpdate) then lovepdaudio.PureDataPlayer_Update(gPDPlayer) end
end
function love.draw () love.graphics.print("hello world",0,0) end

function love.keypressed( key, unicode )
    if (key == "escape") then os.exit(0) end
    if (key == "a") then gDoPDUpdate = not gDoPDUpdate print("gDoPDUpdate",gDoPDUpdate) end
end
