
-- Program to print content of LuaRS232 and LuaRS232.SerialPortOpen(...)
-- Example usage:
-- lua-5.3.1\bin\lua53.exe PrintInterfaceInfo.lua COM12
local PortName=...

-- Function to format the content of a nested table into printable form
-- Properly handle recursive tables!
-- Example usage: print(PrettyPrintTable(_G))
local function PrettyPrintTable(t,n,a)
	-- First argument is table to print
	-- Second argument is how much to indent
	-- Third argument is a list of ancestors, each element being {key,value} of that ancestor
	local s={}--List of string fragments, to concatenate at the end for the complete printable string
	local withtype=false-- Do you want to prefix everything by its type?
	local function fvfp(v,ik)-- Small function to stringify anything, second argument is whether you want [ ] around to make it look like a key.
		local p=type(v)=="string" and "\"" or ""
		return (withtype and ("("..type(v)..") ") or "")..(ik and "[" or "")..p..(type(v)=="function" and "function(...) ... end" or tostring(v))..p..(ik and "]" or "")
	end
	local n=n or 0
	local function Add(txt)
		s[1+#s]=("\t"):rep(n)..txt.."\n"
	end
	if type(t)=="table" then
		Add("{")
		n=n+1
		local maxkeycharlen=0
		local kl={}
		for k,_ in pairs(t) do
			table.insert(kl,k)
			maxkeycharlen=math.max(maxkeycharlen,fvfp(k,true):len())
		end
		table.sort(kl,function(u,v)
			if type(u)==type(v) then
				if type(t[u])==type(t[v]) then
					return u<v
				else
					return type(t[u])<type(t[v])
				end
			else
				return type(u)<type(v)
			end
		end)
		for _,k in ipairs(kl) do
			local v=t[k]
			local alreadyseen=false
			if type(v)=="table" then
				for ak,av in ipairs(a or {}) do
					if av[2]==v then
						alreadyseen=" [===[ same as ancestor -"..(#a-ak+1).." "..fvfp(av[1],true).." ]===],"
					end
				end
			end
			local ks=fvfp(k,true)
			Add(ks..(" "):rep(maxkeycharlen-ks:len()).." ="..(type(v)=="table" and (alreadyseen or "") or " "..fvfp(v)..","))
			if type(v)=="table" and not alreadyseen then
				local function CreateTableShallowCopy(t1)
					local t2={}
					for k,v in pairs(t1) do
						t2[k]=v
					end
					return t2
				end
				local a=CreateTableShallowCopy(a or {})
				a[1+#a]={k,v}
				s[1+#s]=PrettyPrintTable(v,n+1,a)
			end
		end
		n=n-1
		Add("},")
	else
		error("Do want a table")
	end
	return table.concat(s)
end

-- Change table to nicely printed tables, and (possibly, but currently disabled) number display to hexadecimal
local oritostring=tostring
function tostring(v)
	if type(v)=="table" then
		return PrettyPrintTable(v)
	--elseif type(v)=="number" and math.type(v)=="integer" then
	--	return ("0x%X"):format(v)
	else
		return oritostring(v)
	end
end


local LuaRS232=require("LuaRS232")
print("")
print("LuaRS232 = "..tostring(LuaRS232))
print("")
print("")
local UART=LuaRS232.SerialPortOpen(PortName)
print("LuaRS232.SerialPortOpen(...) = "..tostring(UART))
print("")
print("")

