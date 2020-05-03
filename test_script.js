const nightTint = [0.6, 0.55, 1.0];

function applyTint(palette, tint) {
    for (let i = 0; i < palette.length; i++) {
        const color = palette[i];
        for (let j = 0; j < tint.length; j++) {
            color[j] = Math.floor(color[j] * tint[j]);
        }
    }
}

// Porymap callback when a map is opened.
export function on_map_opened(mapName) {
    try {
        for (let i = 0; i < 13; i++) {
            const primaryPalette = map.getPrimaryTilesetPalette(i)
            applyTint(primaryPalette, nightTint)
            map.setPrimaryTilesetPalette(i, primaryPalette)
            const secondaryPalette = map.getSecondaryTilesetPalette(i)
            applyTint(secondaryPalette, nightTint)
            map.setSecondaryTilesetPalette(i, secondaryPalette)
        }
    } catch (err) {
        console.log(err)
    }
}
