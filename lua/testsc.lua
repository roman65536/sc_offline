
--package.cpath = "/usr/lib/lua/5.2/luasql/odbc.so"

--require("luasql.odbc")

--m_sql=require "luasql.odbc"
--env = assert (m_sql.odbc())

--con = assert (env:connect("test"))


package.cpath = "./sc.so"



a=require("sc")



--cur=assert(con:execute("select Test_field1, Test_field2 from roman.test"))




b=a.open("test")
c=a.open("test1")
b:loadxlsx("example/test.xlsx")
c:loadxlsx("/tmp/test2.xlsx")
t=b:getsheets()
for z,n in pairs(t)
 do
  print (z,n)
 end

b:sheet("Roman")
b:recalc()
--b:lexporthtml()

for a=1,1000000 
do

--row= cur:fetch({}, "a")

--e=row.Test_field1
e=a
print ( " Database value " .. e)

b:sheet("Roman")
b:lsetnum(2,0,e)
b:recalc()

val=b:lgetnum(2,1)
--print(val)
val=b:lgetnum(2,0)
--print(string.format("%.5g",val))
val=b:lgetnum(8,0)
--print(string.format("%.5g",val))
val=b:lgetnum(13,0)
print(string.format("ret val %.5g",val))

c:recalc()
val=c:lgetnum(4,1)
print(string.format("Return val %g\n",val))

end
