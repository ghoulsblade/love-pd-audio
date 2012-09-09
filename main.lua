-- test with love2d


function libpdhook (event,...) print("libpdhook",event,...) end

gDoPDUpdate = true
function love.load () 
	print("01")
	require("lovepdaudio")
	print("02")
	lovepdaudio.helloworld() 
	print("03")
end
function love.update () 
	local delay = 100
	gPDPlayer = gPDPlayer or lovepdaudio.CreatePureDataPlayer("pdnes.pd",nil,delay)
	if (gDoPDUpdate) then lovepdaudio.PureDataPlayer_Update(gPDPlayer) end
end
function love.draw () love.graphics.print("hello world",0,0) end

function love.keypressed( key, unicode )
    if (key == "escape") then os.exit(0) end
    if (key == "a") then gDoPDUpdate = not gDoPDUpdate print("gDoPDUpdate",gDoPDUpdate) end
end
