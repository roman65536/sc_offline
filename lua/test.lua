-- before reading this code, please consider the following idioms:
---- variables "r" and "c" always refer to a row and a column.
------ this is specially important in formula functions,
------ as they always must accept a parameter for the row and column.
---- a formula is a lua function registered in the formula_bodies table.
---- most functions meant to be used in formulas accept r,c as first parameters

-- this weak table is meant to enable easy saving of the spreadsheets later.
formula_bodies_mt = {
  __mode = "k"
}
formula_bodies = {}
setmetatable(formula_bodies, formula_bodies_mt)

-- formulas must meet some criteria, the main one being that it must have their
-- body registered as a string in the formula_bodies table (so it can later be
-- saved to a file). This means a formula must always be created using a string,
-- but it can later be copied as a function, when needed.
formula_mt = {
  __newindex = function(t,j,v)
    if type(v) == "string" then
      str =  "func_tmp = function(r,c) " .. v .. " end"
      local f = loadstring(str)
      f()
      if func_tmp == nil then
        error("Syntax error on formula.",2)
      end
      t.func[j] = func_tmp
      t.cell_row[j] = ""
      formula_bodies[func_tmp] = v
      func_tmp = nil
      return
    end
    if type(v) == "function" then
      if formula_bodies[v] == nil then
        error("Function assigned for cell is not a formula.",2)
      end
      t.func[j] = v
      t.cell_row[j] = ""
      return
    end
    if v == nil then
      t.func[j] = nil
      return
    end
    error("Invalid assignment for formula. Type must be a string or a function.",2)
  end,
  __index = function(t,j)
    return t.func[j]
  end,
}

-- this is the metatable for both the formulas and cells "rows tables"
mt = {
  __index = function(t,i)
    t[i] = {}
    if t.is_formula_table then
      t[i].func = {cell_row = t.cells[i]}
      setmetatable(t[i], formula_mt)
      if t.cells[i] == nil then
        t.cells[i] = {}
      end
    else
      t[i]["r"] = i
    end
    return t[i]
  end
}

-- this table is used for buffered update (more details a couple lines below)
local new = {is_formula_table = false} setmetatable(new,mt)
local _sheets = {}

-- by default, column size is 10 characters
local colsize_mt = {
  __index = function()
    return 10
  end
}

-- by default, all rows are visible
local viewrow_mt = {
  __index = function()
    return true
  end
}

-- makes the provided column have the minimum size needed to show its content.
local stretch_to_fit_ = function(c)
  colsize[c] = 0
  for i,v in pairs(cell) do
    if type(v) == "table" and v[c] and #tostring(v[c]) > colsize[c] then
      colsize[c] = #tostring(v[c])
    end
  end
end

function stretch_to_fit(s,e)
  if e == nil then e = s end
  for i=s,e do
    stretch_to_fit_(i)
  end
end

-- creates a sheet and switches to it.
function new_sheet(name)
  cell      = {is_formula_table = false}              setmetatable(cell,mt) 
  formula   = {is_formula_table = true, cells = cell} setmetatable(formula,mt)
  colsize   = {}                                      setmetatable(colsize,colsize_mt)
  view_row  = {}                                      setmetatable(view_row,viewrow_mt)
  info = {
    viewport = {c = 1, r = 1},
    rows_visible = 10,
    columns_visible = 10,
    header = 0,
    margin = 0,
  }
  _sheets[name] = {
    formula_  = formula,
    cell_     = cell,
    colsize_  = colsize,
    view_row_ = view_row,
    info_     = info,
  }
  current_sheet = name
  step()
end

-- drops current sheet
function drop()
  if current_sheet == "" then
    error("No sheet selected.",2)
  end
  formula = nil
  cell = nil
  local s = _sheets[current_sheet]
  s.formula_ = nil
  s.cell_    = nil
  _sheets[current_sheet] = nil
  current_sheet = ""
  for k in pairs(_sheets) do
    switch(k)
    return true
  end
end

-- list all sheets
function sheets()
  for k in pairs(_sheets) do print(k) end
end

-- changes current sheet
function switch(sheet)
  local s = _sheets[sheet]
  if type(s) == "table" and s.formula_ and s.cell_ then
    formula  = s.formula_
    cell     = s.cell_
    colsize  = s.colsize_
    view_row = s.view_row_
    info     = s.info_
    current_sheet = sheet
    step()
    return true
  end
  error("Sheet does not exist.",2)
end

local err_mt = {
  __tostring = function()
    return "#ERR#"
  end
}

-- make an "exception table" and returns it.
function make_formula_error(val)
  local formula_error = {str = val}
  setmetatable(formula_error,err_mt)
  return formula_error
end

function update_all()
  if current_sheet == "" then
    error("No sheet selected.",2)
  end
  for row_index,row in pairs(cell) do
    if type(row) == "table" then
      for column_index in pairs(row) do
        local func = formula[row_index][column_index]
        if func then
          local ok, val = pcall(func, row_index, column_index)
          if ok then
            new[row_index][column_index] = val
          else
            new[row_index][column_index] = make_formula_error(val)
          end
        end
      end
    end
  end
  for row_index,row in pairs(cell) do
    if type(row) == "table" then
      for column_index in pairs(row) do
        local func = formula[row_index][column_index]
        if func then
          cell[row_index][column_index] = new[row_index][column_index]
        end
      end
    end
  end
  return true
end

-- The default behavior is to buffer the updated values before, which allows for
-- more consistent behavior for circular dependencies, but it's slower. An
-- unbufered, faster option is provided should the user need it.
local buffered_update_ = {
  [true]  = update_all,
  [false] = function()
    if current_sheet == "" then
      error("No sheet selected.",2)
    end
    for row_index,row in pairs(cell) do
      if type(row) == "table" then
        for column_index in pairs(row) do
          local func = formula[row_index][column_index]
          if func then
            local ok, val = pcall(func, row_index, column_index)
            if ok then
              cell[row_index][column_index] = val
            else
              cell[row_index][column_index] = make_formula_error(val)
            end
          end
        end
      end
    end
    return true
  end
}

-- switches the update mode from buffered(true) to unbufered(false)
function buffered_update(option)
  update_all = buffered_update_[option]
end

header = function(n)
  info.header = n
end

-- sets the first row and column that is visible on the terminal.
viewport = function(r,c)
  local r_s = 99999999999999 - info.rows_visible + 1
  local c_s = 99999999999999 - info.columns_visible + info.header + 1
  r = r < r_s and r or r_s
  c = c < c_s and c or c_s
  info.viewport.r = r > 0 and r or 1
  info.viewport.c = c > 0 and c or 1
end

-- sets the number of rows that are visible
rows_visible = function(val)
  info.rows_visible = val
  viewport(info.viewport.r,info.viewport.c)
end

-- sets the number of columns that are visible
columns_visible = function(val)
  info.columns_visible = val
  viewport(info.viewport.r,info.viewport.c)
end

-- advances `rows_visible` rows on the viewport
next_page = function()
  viewport(info.viewport.r + info.rows_visible - info.header,info.viewport.c)
  print_sheet()
end

-- go back `rows_visible` rows on the viewport
previous_page = function()
  viewport(info.viewport.r - info.rows_visible + info.header,info.viewport.c)
  print_sheet()
end

-- This big, cumbersome function, that needs a lot of refactoring, prints the
-- sheet according to the current viewport.
function print_sheet()
  local r = info.viewport.r
  local c = info.viewport.c
  local e = info.columns_visible-1
  local first_col_size = #tostring(r+info.rows_visible-1)+2
  if current_sheet == "" then
    error("No sheet selected.",2)
  end
  print('Sheet "' .. current_sheet .. '":')
  local i = c
  io.write(string.rep(" ",first_col_size))
  while i <= (c+e) do
    local str = tostring(i)
    local fill_size = colsize[i]-#str
    if #str > colsize[i] then
      if colsize[i] > 1 then
        str = "<" .. string.sub(str,-colsize[i]+1,-1)
      else
        str = string.rep("<",colsize[i]-1)
      end
    end
    if colsize[i] > 0 then
      io.write("{" .. string.rep(" ",fill_size) .. str .. "}")
    else
      e = e+1
    end
    i = i+1
  end
  io.write("\n")
  -- I define a function to avoid code duplication below.
  print_row = function(tb)
    if view_row[tb.i] then
      c_size = first_col_size-#tostring(tb.i)-2
      io.write("{" .. string.rep(" ",c_size) .. tb.i .. "}")
      if cell[tb.i] then
        for j = c,(c+e) do
          if colsize[j] > 0 then
            io.write("[")
            local val = tostring(cell[tb.i][j])
            local str
            if #val > colsize[j] then
              str = string.sub(tostring(cell[tb.i][j]),1,colsize[j]-1) .. ">"
            else
              str = val
            end
            local fill_size = colsize[j] - #str
            if cell[tb.i][j] then
              if type(cell[tb.i][j]) ~= "number" then
                io.write(str .. string.rep(" ",fill_size))
              else
                io.write(string.rep(" ",fill_size) .. str)
              end
            else
              io.write(string.rep(" ",colsize[j]))
            end
            io.write("]")
          end
        end
      else
        for j = 1,info.columns_visible do io.write("[" .. string.rep(" ",colsize[j]) .. "]") end
      end
      io.write("\n")
    else
      tb.total = tb.total + 1
    end
    tb.i = tb.i+1
  end
  -- I only use a table here so I can "pass by reference". Is this a bad idea?
  local tb = {
    i = 1,
    total = info.header,
  }
  while tb.i <= tb.total do -- print header rows.
    print_row(tb)
  end
  tb = {
    i = r > info.header and r or info.header+1,
    total = r+info.rows_visible-1-info.header,
  }
  while tb.i <= tb.total do -- print visible rows.
    print_row(tb)
  end
  return true
end

-- updates all cells and prints the current sheet.
function step(n)
  if current_sheet == "" then
    error("No sheet selected.",2)
  end
  if n == nil then n = 1 end
  for i=1,n do
    update_all()
  end
  print_sheet()
  return true
end

-- returns the value of the cell
function getcell(sheet,r,c)
  return _sheets[sheet].cell_[r][c]
end

-- an error in a cell is displayed as #ERR#. This function can get the string for it.
function geterror(r,c)
  return cell[r][c].str
end

-- returns the formula string of the cell
function getformula(r,c)
  return formula_bodies[formula[r][c]]
end

-- I would really love to avoid the duplication here.
function drag(r,c,direction,n,e)
  if e == nil then e = 1 end
  if direction == "right" then
    for k = r,(r+e-1) do
      for i=(c+1),(c+n) do
        cell[k][i] = cell[k][c]
        formula[k][i] = formula[k][c]
      end
    end
    return
  end
  if direction == "left" then
    for k = r,(r+e-1) do
      for i=(c-1),(c-n),-1 do
        cell[k][i] = cell[k][c]
        formula[k][i] = formula[k][c]
      end
    end
    return
  end
  if direction == "down" then
    for k = c,(c+e-1) do
      for i=(r+1),(r+n) do
        cell[i][k] = cell[r][k]
        formula[i][k] = formula[r][k]
      end
    end
    return
  end
  if direction == "up" then
    for k = c,(c+e-1) do
      for i=(r-1),(r-n) do
        cell[i][k] = cell[r][k]
        formula[i][k] = formula[r][k]
      end
    end
    return
  end
end

-- FUNCTIONS hfill, vfill
-- Will fill the row/column starting at r,c with all parameters, skipping the cell
-- where the parameter is 'nil'
function hfill(r,c,...)
  local params = {...}
  for i,v in pairs(params) do
    cell[r][c+i-1] = v
  end
end

function vfill(r,c,...)
  local params = {...}
  for i,v in pairs(params) do
    cell[r+i-1][c] = v
  end
end

-- FUNCTIONS seq_vfind, seq_hfind
-- Similar functionality to excel VLOOKUP
-- This does a sequential scan and is only useful if the dataset is not ordered.
-- (bin_vfind and bin_hfind are still to be implemented)
-- PARAMETERS:
-- sheet: name of the sheet to search into
-- column: id of the column to search into
-- result: id of the column to extract the value from
-- start: the row to start the search(inclusive)
-- finish: the row to finish the search(inclusive)
-- predicate: the value to search for, or a function that returns
--            true if the value to search for is passed as an argument
function seq_vfind(sheet,column,result,start,finish,predicate)
  if predicate == nil then
    error("seq_vfind: predicate can't be 'nil'",2)
  end
  if type(predicate) ~= "function" then
    local p = predicate
    predicate = function(val)
      return val == p
    end
  end
  local s = _sheets[sheet]
  if s == nil then
    error("seq_vfind: sheet does not exist",2)
  end
  for i=start,finish do
    if predicate(s.cell_[i][column]) then
      return s.cell_[i][result]
    end
  end
  error("seq_vfind: key not found",2)
end

function seq_hfind(sheet,row,result,start,finish,predicate)
  if predicate == nil then
    error("seq_hfind: predicate can't be 'nil'",2)
  end
  if type(predicate) ~= "function" then
    local p = predicate
    predicate = function(val)
      return val == p
    end
  end
  local s = _sheets[sheet]
  if s == nil then
    error("seq_hfind: sheet does not exist",2)
  end
  for i=start,finish do
    if predicate(s.cell_[row][i]) then
      return s.cell_[result][i]
    end
  end
  error("seq_hfind: key not found",2)
end


-- let's create a sample document for demonstration purposes. All of this could be done easily in interactive mode.
new_sheet("students")
hfill(1,1,"registration","name") -- fills the header
-- now let's register some students ...
vfill(2,1,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15) -- still need a function to make sequences :D
vfill(2,2,"Mary","Luke","Anna","Tony","Bobby Tables","Liz","Scarlet",
          "Cassio","Paul","Nadia","Shosshana","Brad","Lilly","John","Carlos")
step() -- stepping is not really needed since there are no formulas. This will just print the sheet.
-- ... and input the grades for the ones with an odd number
new_sheet("grades")
hfill(1,1,"registration","name","grade 1","grade 2","total","situation")
vfill(2,1,1,3,5,7,9,11,13,15)
formula[2][2] = "return seq_vfind('students',1,2,2,16,cell[r][c-1])" -- look for the student's name in the students sheet.
drag(2,2,"down",8) -- copies the cell[2][2] content(in this case, the formula) though 8 cells below it. the 8th cell is just to showcase the error.
vfill(2,3,2.5,3.2,5.5,.5,6,10,4.1,8.8) -- 1st grades
vfill(2,4,3.2,3,9.5,.2,6.5,10,4.9,7)   -- 2nd grades
formula[2][5] = "return (cell[r][c-2]+cell[r][c-1])/2"
drag(2,5,"down",7)
formula[2][6] = "if cell[r][c-1]<8 then return 'FAILED' else return 'PASSED' end"
drag(2,6,"down",7)
update_all()
formula[10][3] = "return geterror(r,c-1)"
update_all()
stretch_to_fit(1)
stretch_to_fit(2)
stretch_to_fit(3)
header(1) -- size of the header is 1 row. (check effect using next_page())
rows_visible(6)
step()
print("type next_page() to view more, or type rows_visible(11) -- then step() -- to view all rows at once")
