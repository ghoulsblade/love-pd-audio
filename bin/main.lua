-- test with love2d

love.filesystem.load("lib.pdnet.lua")()

function libpdhook (event,...) print("libpdhook",event,...) end

gDoPDUpdate = true

gSliders = {}
gButtons = {}

function love.load () 
	local bEnablePDLib = true
	local delay = 100
	local filepath,dir = "test.pd",nil
	local filepath,dir = "pdnes.pd",nil
	local filepath,dir = "MAIN_musicmachine.pd","musicmaschiene_v1"
	local filepath,dir = "MAIN_beatmachine.pd","beatmachine-v2"
	local filepath,dir = "MAIN_musicmachine_v2.pd","musicmaschiene_v2"
	--~ local filepath,dir = "samplebank.pd","sample_bank"
	--~ local filepath,dir = "samplebank_auto.pd","sample_bank"
	--~ local filepath,dir = "samplebank_lovenet.pd","sample_bank"
	
	if (filepath == "samplebank_lovenet.pd") then  bEnablePDLib = false  end
	
	print("lua _VERSION",_VERSION)
	print("01")
	if (bEnablePDLib) then require("lovepdaudio") end
	print("02")
	if (bEnablePDLib) then lovepdaudio.helloworld() end
	print("03")
	if (not bEnablePDLib) then 
		-- make dummy
		lovepdaudio = {}
		lovepdaudio.CreatePureDataPlayer = function () return {} end
		lovepdaudio.PureDataPlayer_Update = function () end
		libpd_add_to_search_path = function () end
		libpd_float = function (receiver_name,val) PDNet:SendFloat(receiver_name,val) end
		libpd_bang = function () end
		libpd_bind = function () end
	end
	
	print("start playback...")
	
	libpd_bind("samplesize")
	libpd_bind("speedorig")
	libpd_bind("playpos")
	libpd_add_to_search_path("rj/")
	
	gPDPlayer = lovepdaudio.CreatePureDataPlayer(filepath,dir,delay)
	
	if (filepath == "MAIN_musicmachine.pd") then libpd_float("loopspeed",0.023) end
	if (filepath == "MAIN_musicmachine_v2.pd") then libpd_float("loopspeed",0.124) end
	if (filepath == "samplebank.pd") then libpd_float("dsp",0) libpd_bang("myloadsamp") libpd_float("dsp",1) end
	if (filepath == "samplebank_auto.pd") then  libpd_float("dsp",1) end
	if (filepath == "samplebank_lovenet.pd") then  libpd_float("dsp",1) end
	
	-- some gfx
	love.graphics.setBackgroundColor( 0x83,0xc0,0xf0 ) -- love blue from wiki
	gSliders.speed	= { x=40*0+10,y=20,w=20,h=200, min=0,max=0.27,cur=0.124, on_change=function (v) print("loopspeed",v) libpd_float("loopspeed",v) end}
	gSliders.bpm	= { x=40*1+10,y=20,w=20,h=200, min=100,max=300,cur=200, on_change=function (v) print("bpm",v) libpd_float("bpm",v) end}
	
	gButtons.sample	= { x=60*0+10,y=260, w=60,h=20, label="sample", on_press=function () libpd_bang("playsample") end}
	gButtons.play	= { x=60*1+10,y=260, w=60,h=20, label="play", on_press=function () libpd_bang("myplay") end}
	for i=0,12 do gButtons["a"..i] = { x=20*i+10,y=290, w=20,h=20, label=""..i, on_press=function () libpd_float("mybuttons",i) end} end
	
	print("PDNet:Init() ...")
	PDNet:Init()
	
	print("(init done)")
end

function love.mousepressed( x, y, button )
	if (button == "l") then 
		for k,o in pairs(gButtons) do 
			if (x >= o.x and y >= o.y and x <= o.x+o.w and y <= o.y+o.h) then o.pressed = 5 o.on_press() end
		end
	end
end

function love.keypressed( key, unicode )
    if (key == "escape") then os.exit(0) end
    if (key == "a") then gDoPDUpdate = not gDoPDUpdate print("gDoPDUpdate",gDoPDUpdate) end
    if (key == "1") then libpd_bang("playsample") end
    if (key == "2") then libpd_float("loopspeed",0.023) end
    if (key == "3") then libpd_bang("originalspeed") end
    if (key == "4") then libpd_bang("myloopplay") end	
    if (key == "5") then libpd_bang("test2 bang") end	
    if (key == "6") then PDNet:SendBang("test2") end	
end

function love.update ()
	if (gDoPDUpdate) then lovepdaudio.PureDataPlayer_Update(gPDPlayer) end
	local mx = love.mouse.getX()
	local my = love.mouse.getY()
	
	if (love.mouse.isDown("l")) then 
		for k,o in pairs(gSliders) do 
			if (mx >= o.x and mx <= o.x+o.w and
				my >= o.y and my <= o.y+o.h) then 
				local curnew = o.min + (o.max - o.min)*(1 - (my - o.y) / o.h)
				if (o.cur ~= curnew) then 
					o.cur =  curnew
					o.on_change(o.cur)
				end
			end
		end
	end
end

function love.draw () 
	love.graphics.print("hello world",0,0)
	
	for k,o in pairs(gButtons) do 
		local mode = "line"
		if ((o.pressed or 0) > 0) then mode = "fill" o.pressed = o.pressed - 1 end
		love.graphics.rectangle( mode, o.x, o.y, o.w, o.h )
		love.graphics.print(o.label or "",o.x+2,o.y+2)
	end
	
	for k,o in pairs(gSliders) do 
		love.graphics.rectangle( "line", o.x, o.y, o.w, o.h )
		local x = o.x + 0.5*o.w
		local f = (o.cur - o.min) / (o.max - o.min)
		local y = o.y + ( 1 - f ) * o.h 
		love.graphics.circle( "line", x, y, 0.5*o.w )
		love.graphics.print(string.format("%0.2f",o.cur),o.x,o.y+o.h+10)
	end
end


