-- Teal loader for Mushkin
-- Automatically compiles and loads .tl files when required
-- Usage: require("teal_loader") at startup, then require("myfile") will find myfile.tl

local tl = require("tl")

-- Create a package loader that handles .tl files
local function teal_loader(name)
    -- Convert module name to path (e.g., "foo.bar" -> "foo/bar")
    local path_name = name:gsub("%.", "/")

    -- Search for .tl file in package.path locations
    local tl_path = package.path:gsub("%.lua", ".tl")
    local filepath = package.searchpath(name, tl_path)

    if not filepath then
        return nil  -- Not found, let other loaders try
    end

    -- Read the .tl file
    local file, err = io.open(filepath, "r")
    if not file then
        return nil, "Cannot open " .. filepath .. ": " .. (err or "unknown error")
    end

    local source = file:read("*a")
    file:close()

    -- Compile Teal to Lua
    local lua_code, result = tl.gen(source)

    if not lua_code then
        local errors = {}
        if result and result.syntax_errors then
            for _, e in ipairs(result.syntax_errors) do
                table.insert(errors, e.msg or tostring(e))
            end
        end
        if result and result.type_errors then
            for _, e in ipairs(result.type_errors) do
                table.insert(errors, e.msg or tostring(e))
            end
        end
        error("Teal compile error in " .. filepath .. ":\n" .. table.concat(errors, "\n"))
    end

    -- Load the compiled Lua code
    local chunk, load_err = loadstring(lua_code, "@" .. filepath)
    if not chunk then
        error("Lua load error in " .. filepath .. ":\n" .. (load_err or "unknown error"))
    end

    return chunk
end

-- Insert our loader after the preload loader (position 2)
-- This ensures .tl files are found before falling back to C loaders
table.insert(package.loaders, 2, teal_loader)

-- Also provide tl module functions for direct use
return tl
