
print("01")
require("lovepdaudio")
print("02")
lovepdaudio.helloworld() 
print("03")

function love.load () end
function love.draw () love.graphics.print("hello world",0,0) end
