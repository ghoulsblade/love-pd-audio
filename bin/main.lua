-- test with love2d


function libpdhook (event,...) print("libpdhook",event,...) end

gDoPDUpdate = true
function love.load () 
	print("lua _VERSION",_VERSION)
	print("01")
	require("lovepdaudio")
	print("02")
	lovepdaudio.helloworld() 
	print("03")
	print("(init done)")
end
function love.update () 
	local delay = 100
	local filepath = "test.pd"
	local filepath = "pdnes.pd"
	gPDPlayer = gPDPlayer or lovepdaudio.CreatePureDataPlayer(filepath,nil,delay)
	if (gDoPDUpdate) then lovepdaudio.PureDataPlayer_Update(gPDPlayer) end
end
function love.draw () love.graphics.print("hello world",0,0) end

function love.keypressed( key, unicode )
    if (key == "escape") then os.exit(0) end
    if (key == "a") then gDoPDUpdate = not gDoPDUpdate print("gDoPDUpdate",gDoPDUpdate) end
end
