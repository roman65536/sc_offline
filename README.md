Copy of Roman sc_offline

### Batch Spreadsheet

Batch Spreadsheet or Offline Spreadsheet, can be used calculating spreadsheets as batch processing.
This means without any interface (cli,gui). Realised is Batch Spreadsheet as a lua module(5.2) and 
with lua you pass over cells values to the Batch Spreadsheet, which are then calculated.   
As template a excel xml sheet can by used. Batch Spreadsheet so far supports basic math functions such as 
max(),min(),sum(),prod(),avg(). 

#### Idea behind Batch Spreadsheet

Excel is widly acceptet Spreadsheet in the World and almost everybody can use it. So why not use Excel files as somesort of Template for calculating other stuff.
Imagine you have a Database with data and you like to do some calculations on the data values and you know exactly what and how using excel. Yes you can do this using Excel and VB as well, but lets asume, you like to do the same thing on a Linux without any gui or cli intervention. 
Now since english is not my native langague it is hard to explain it in details. Lets try following example:


```
a=require("sc")
..

b=a.open("test")
b:loadxlsx("/tmp/test.xlsx")
..
...
cur=assert(con:execute("select value,sensor from sensors order by datetime limit 10000"))



b:sheet("Roman") -- Select Sheet and set it active 

row= cur:fetch({}, "a")
while row do
val=tonumber(row.value)
print ( " Database value " .. row.sensor)

b:lsetnum(2,0,val)  -- Excel sheet A2 value is set with the data from DB 
b:recalc()  -- recalc the sheet

val=b:lgetnum(8,0)   -- Get the values from A8 and do with something
print(string.format("%.5g",val))
end

```

Hope you get the idea. Yes you can load more sheets at once and more the one document. Only memory is your limit.

 

 



