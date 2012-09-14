-- test with love2d


function libpdhook (event,...) print("libpdhook",event,...) end

gDoPDUpdate = true

gSliders = {}

function love.load () 
	print("lua _VERSION",_VERSION)
	print("01")
	require("lovepdaudio")
	print("02")
	lovepdaudio.helloworld() 
	print("03")
	
	
	print("start playback...")
	
	local delay = 100
	local filepath = "test.pd"
	local filepath = "pdnes.pd"
	local filepath = "MAIN_musicmachine.pd"
	libpd_bind("samplesize")
	libpd_bind("speedorig")
	-- libpd_bind("playpos")
	gPDPlayer = lovepdaudio.CreatePureDataPlayer(filepath,nil,delay)
	libpd_float("loopspeed",0.023)
	
	-- some gfx
	love.graphics.setBackgroundColor( 0x83,0xc0,0xf0 ) -- love blue from wiki
	gSliders.speed = { x=10,y=20,w=20,h=200, min=0,max=0.07,cur=0.023, on_change=function (v) print("loopspeed",v) libpd_float("loopspeed",v) end}
		
	print("(init done)")
end

function love.keypressed( key, unicode )
    if (key == "escape") then os.exit(0) end
    if (key == "a") then gDoPDUpdate = not gDoPDUpdate print("gDoPDUpdate",gDoPDUpdate) end
    if (key == "1") then libpd_bang("playsample") end
    if (key == "2") then libpd_float("loopspeed",0.023) end
    if (key == "3") then libpd_bang("originalspeed") end
    if (key == "4") then libpd_bang("myloopplay") end	
end

function love.update ()
	if (gDoPDUpdate) then lovepdaudio.PureDataPlayer_Update(gPDPlayer) end
	local mx = love.mouse.getX()
	local my = love.mouse.getY()
	
	if (love.mouse.isDown("l")) then 
		for k,o in pairs(gSliders) do 
			if (mx >= o.x and mx <= o.x+o.w and
				my >= o.y and my <= o.y+o.h) then 
				o.cur = o.min + (o.max - o.min)*(1 - (my - o.y) / o.h)
				o.on_change(o.cur)
			end
		end
	end
end

function love.draw () 
	love.graphics.print("hello world",0,0)
	
	for k,o in pairs(gSliders) do 
		love.graphics.rectangle( "line", o.x, o.y, o.w, o.h )
		local x = o.x + 0.5*o.w
		local f = (o.cur - o.min) / (o.max - o.min)
		local y = o.y + ( 1 - f ) * o.h 
		love.graphics.circle( "line", x, y, 0.5*o.w )
		love.graphics.print(string.format("%0.2f",o.cur),o.x,o.y+o.h+20)
	end
end


