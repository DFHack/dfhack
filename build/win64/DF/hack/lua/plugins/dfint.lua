local _ENV = mkmodule('plugins.dfint')

local overlay = require('plugins.overlay')

DfintOverlay = defclass(DfintOverlay, overlay.OverlayWidget)
DfintOverlay.ATTRS {
    viewscreens = 'all',
    default_enabled = true,
    overlay_only = true,
}

function DfintOverlay:onRenderFrame(dc)
    renderOverlay()
end

function load_font(font)
    loadFont(font .. '.ttf')
end

function disable_font()
    disableFont()
end

function get_font_files(font_dir)
    local files = {}
    print('searching ' .. font_dir)
    for _, entry in ipairs(dfhack.filesystem.listdir_recursive(font_dir)) do
        if not entry.isdir and entry.path:lower():endswith('.ttf') then
            table.insert(files, entry.path:sub(3, -5))
            print('file ' .. entry.path)
        end
    end
    table.sort(files)
    return files
end

OVERLAY_WIDGETS = { dfint = DfintOverlay }

return _ENV
