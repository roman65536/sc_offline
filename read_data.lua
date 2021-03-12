
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
b:loadxlsx("/tmp/TAFLIR_Gesamtliste.xlsx")
t=b:getsheets()
for z,n in pairs(t)
 do
  print (z,n)
  lltype=ltype[n];
  print(lltype);

v=b:sheet(n)

for a=3, b:lmaxrow()
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
val5=b:lgetnum(3,a)
val2=b:lgetnum(10,a)
val3=b:lgetnum(13,a)

if val2 == nil then val2 =0
 end

print(string.format("%s %s %s %s\n",val1,val5,val2,val3));

val3=val3-25569
val3=val3*86400
val4=b:lgetstr(4,a)
--print(string.format("%d-%d %f %s \"%s\"",val1,val5,val2,os.date('%Y-%m-%d %H:%M:%S',val3),val4))
print(string.format("insert into auftrag values (%d,%d,%f,'%s','%s');",val1,val5,val2,os.date('%Y-%m-%d',val3),PostgresEscape(val4)))
con:execute(string.format("insert into auftrag values (%d,%d,%f,'%s','%s','%s');",val1,val5,val2,os.date('%Y-%m-%d',val3),PostgresEscape(val4),lltype))
--val=b:lgetnum(0,8)
--print(string.format("%.5g",val))
--val=b:lgetnum(0,13)
--print(string.format("ret val %d",val))
--val=b:lgetnum(10,13)
--print(string.format("ret val %d",val))

end

end
