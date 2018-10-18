
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
c:loadxlsx("example/test2.xlsx")
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

v=b:sheet("Roman")
print( v  )
b:lsetnum(0,2,e)
b:recalc()

val=b:lgetnum(1,2)
--print(val)
val=b:lgetnum(0,2)
--print(string.format("%.5g",val))
val=b:lgetnum(0,8)
--print(string.format("%.5g",val))
val=b:lgetnum(0,13)
print(string.format("ret val %.5g",val))

c:recalc()
val=c:lgetnum(1,4)
print(string.format("Return val %g\n",val))

end
