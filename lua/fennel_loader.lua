-- Fennel loader for Mushkin
-- Automatically compiles and loads .fnl files when required
-- Usage: require("fennel_loader") at startup, then require("myfile") will find myfile.fnl

local fennel = require("fennel")

-- Create a package loader that handles .fnl files
local function fennel_loader(name)
    -- Convert module name to path (e.g., "foo.bar" -> "foo/bar")
    local path_name = name:gsub("%.", "/")

    -- Search for .fnl file in package.path locations
    local fnl_path = package.path:gsub("%.lua", ".fnl")
    local filepath = package.searchpath(name, fnl_path)

    if not filepath then
        return nil  -- Not found, let other loaders try
    end

    -- Read the .fnl file
    local file, err = io.open(filepath, "r")
    if not file then
        return nil, "Cannot open " .. filepath .. ": " .. (err or "unknown error")
    end

    local source = file:read("*a")
    file:close()

    -- Compile Fennel to Lua
    local ok, lua_code = pcall(fennel.compileString, source, {
        filename = filepath,
    })

    if not ok then
        error("Fennel compile error in " .. filepath .. ":\n" .. (lua_code or "unknown error"))
    end

    -- Load the compiled Lua code
    local chunk, load_err = loadstring(lua_code, "@" .. filepath)
    if not chunk then
        error("Lua load error in " .. filepath .. ":\n" .. (load_err or "unknown error"))
    end

    return chunk
end

-- Insert our loader after the preload loader (position 2)
-- This ensures .fnl files are found before falling back to C loaders
table.insert(package.loaders, 2, fennel_loader)

-- Also provide fennel module functions for direct use
return fennel
