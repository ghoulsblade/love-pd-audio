-- test with love2d

print("01")
require("lovepdaudio")
print("02")
lovepdaudio.helloworld() 
print("03")

function love.load () 
	gPDPlayer = lovepdaudio.CreatePureDataPlayer("pdnes.pd")
end
function love.update () 
	lovepdaudio.PureDataPlayer_Update(gPDPlayer)
end
function love.draw () love.graphics.print("hello world",0,0) end

function love.keypressed( key, unicode )
    if (key == "escape") then os.exit(0) end
end
