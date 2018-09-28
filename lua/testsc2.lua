package.cpath = "./sc.so"
a=require("sc")
b=a.open("test")

b:loadxlsx("test.xlsx")
t=b:getsheets()

--print sheet names
for z,n in pairs(t)
do
  print (z,n)
end

b:sheet("Tabelle1")
b:recalc()

val=b:lgetnum(4,0)
print(string.format("ret val %.5g",val))

