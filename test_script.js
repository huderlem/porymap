// Porymap callback when a block is painted.
export function on_block_changed(x, y, prevBlock, newBlock) {
    map.clearOverlay()
    map.addFilledRect(0, 0, map.getWidth() * 16 - 1, map.getHeight() * 16 - 1, "#80FF0040")
    map.addRect(10, 10, 100, 30, "#FF00FF")
    map.addImage(80, 80, "D:\\cygwin64\\home\\huder\\scratch\\github-avatar.png")
    map.addText(`coords ${x}, ${y}`, 20, 20, "#00FF00", 24)
    map.addText(`block ${prevBlock.metatileId}`, 20, 60, "#00FFFF", 18)
    console.log("ran", x, y)
}

// Porymap callback when a map is opened.
export function on_map_opened(mapName) {
    map.clearOverlay()
    map.addFilledRect(0, 0, map.getWidth() * 16 - 1, map.getHeight() * 16 - 1, "#4000FF00")
    console.log(`opened ${mapName}`)
}
