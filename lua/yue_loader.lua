-- YueScript loader for Mushkin
-- Automatically compiles and loads .yue files when required
-- Usage: require("yue_loader") at startup, then require("myfile") will find myfile.yue

local yue = require("yue")

-- Create a package loader that handles .yue files
local function yue_loader(name)
    -- Convert module name to path (e.g., "foo.bar" -> "foo/bar")
    local path_name = name:gsub("%.", "/")

    -- Search for .yue file in package.path locations
    local yue_path = package.path:gsub("%.lua", ".yue")
    local filepath = package.searchpath(name, yue_path)

    if not filepath then
        return nil  -- Not found, let other loaders try
    end

    -- Read the .yue file
    local file, err = io.open(filepath, "r")
    if not file then
        return nil, "Cannot open " .. filepath .. ": " .. (err or "unknown error")
    end

    local source = file:read("*a")
    file:close()

    -- Compile YueScript to Lua
    local lua_code, compile_err = yue.to_lua(source, {
        implicitReturnRoot = true,
        reserveLineNumber = true,
        lint_global = false,
    })

    if not lua_code then
        error("YueScript compile error in " .. filepath .. ":\n" .. (compile_err or "unknown error"))
    end

    -- Load the compiled Lua code
    local chunk, load_err = loadstring(lua_code, "@" .. filepath)
    if not chunk then
        error("Lua load error in " .. filepath .. ":\n" .. (load_err or "unknown error"))
    end

    return chunk
end

-- Insert our loader after the preload loader (position 2)
-- This ensures .yue files are found before falling back to C loaders
table.insert(package.loaders, 2, yue_loader)

-- Also provide yue module functions for direct use
return yue
