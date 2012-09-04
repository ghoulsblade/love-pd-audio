-- test with love2d

print("01")
require("lovepdaudio")
print("02")
lovepdaudio.helloworld() 
print("03")
--~ lovepdaudio.test01() 
lovepdaudio.test02("pdnes.pd") 
print("04")

function love.load () end
function love.draw () love.graphics.print("hello world",0,0) end
