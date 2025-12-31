-- MoonScript loader for Mushkin
-- Automatically compiles and loads .moon files when required
-- Usage: require("moonscript_loader") at startup, then require("myfile") will find myfile.moon

local moonscript = require("moonscript")

-- Create a package loader that handles .moon files
local function moonscript_loader(name)
    -- Convert module name to path (e.g., "foo.bar" -> "foo/bar")
    local path_name = name:gsub("%.", "/")

    -- Search for .moon file in package.path locations
    local moon_path = package.path:gsub("%.lua", ".moon")
    local filepath = package.searchpath(name, moon_path)

    if not filepath then
        return nil  -- Not found, let other loaders try
    end

    -- Read the .moon file
    local file, err = io.open(filepath, "r")
    if not file then
        return nil, "Cannot open " .. filepath .. ": " .. (err or "unknown error")
    end

    local source = file:read("*a")
    file:close()

    -- Compile MoonScript to Lua
    local lua_code, line_table_or_err = moonscript.to_lua(source)

    if not lua_code then
        error("MoonScript compile error in " .. filepath .. ":\n" .. (line_table_or_err or "unknown error"))
    end

    -- Load the compiled Lua code
    local chunk, load_err = loadstring(lua_code, "@" .. filepath)
    if not chunk then
        error("Lua load error in " .. filepath .. ":\n" .. (load_err or "unknown error"))
    end

    return chunk
end

-- Insert our loader after the preload loader (position 2)
-- This ensures .moon files are found before falling back to C loaders
table.insert(package.loaders, 2, moonscript_loader)

-- Also provide moonscript module functions for direct use
return moonscript
