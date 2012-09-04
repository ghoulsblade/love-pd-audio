-- test with raw lua

print("01")
require("lovepdaudio")
print("02")
lovepdaudio.helloworld() 
print("03")
--~ lovepdaudio.test01() 
--~ lovepdaudio.test02("test.pd") 
lovepdaudio.test02("pdnes.pd") 
print("04")
