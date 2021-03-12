
 psql=require "luasql.postgres"
print(_VERSION)
 m_env = psql.postgres()
print(m_env)
con = m_env:connect("user=roman password=roman dbname=mydb hostaddr=127.0.0.1 port=5432")
print( con)



package.cpath = "./sc.so"



ltype={};
ltype["IH Planbar YPM1"]="IH Planbar";
ltype["IH nicht Planbar YPM2"]="IH Nicht Planbar";
ltype["Änderungsdienst YPM3"]="Aenderung";
ltype["Material YPM4"]="Material";
ltype["B&B YPM5"]="BB";
ltype["Systembetreuung YPM9"]="Systembetreuung";
ltype["Revision YWA1"]="Revision";
ltype["Rog Grün YWA2"]="RED";



local function PostgresEscape(Value)
   if Value == nil 
	then
	return ""
   else
   return Value:gsub("['\\]", {["'"] = "''", ["\\"] = "\\\\"})
  end
end
 

a=require("sc")



--cur=assert(con:execute("select Test_field1, Test_field2 from roman.test"))



b=a.open("test")
b:loadxlsx("/tmp/2017_1996_2020.09_TAF_WDU_Verfuegbarkeit_v1.0.xlsx")
t=b:getsheets()
for z,n in pairs(t)
 do
  print (z,n)

 end
v=b:sheet("TAF WDU, 1996-9.2020")


b:recalc()
b:recalc()
b:recalc()

for a=7, 30 
do
print(a)
--row= cur:fetch({}, "a")

--e=row.Test_field1
--e=a
--print ( " Database value " .. e)
--v=b:sheet(n)
--v=b:sheet("Rog Grün YWA2")
--print( v  )
--b:lsetnum(0,2,e)
--b:recalc()

val1=b:lgetnum(0,a)
if (val1 == nil) 
 then
 val1=b:lgetstr(0,a)
end
year=tonumber(val1)
if (year < 1900) then
if(year < 90) then year=year+2000
 else
   year = year + 1900
end
end
val2=b:lgetnum(1,a)
val3=b:lgetnum(2,a)
val4=b:lgetnum(3,a)
val5=b:lgetnum(4,a)


print(string.format("%s %d %s %s %s %s \n",val1,year,val2,val3,val4,val5));

sql=string.format("insert into avail_y values(%d,%.2f,%.2f,%.1f);",year,val3,val4,val2)
print(sql)
con:execute(sql)


end
