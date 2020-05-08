const normalTint = [1, 1, 1]
const morningTint = [0.8, 0.7, 0.9];
const nightTint = [0.6, 0.55, 1.0];

function applyTint(palette, tint) {
    for (let i = 0; i < palette.length; i++) {
        const color = palette[i];
        for (let j = 0; j < tint.length; j++) {
            color[j] = Math.floor(color[j] * tint[j]);
        }
    }
}

function applyTintToPalettes(tint) {
    try {
        // const primaryPalettes = map.getPrimaryTilesetPalettes()
        // for (let i = 0; i < primaryPalettes.length; i++)
        //     applyTint(primaryPalettes[i], tint)

        // const secondaryPalettes = map.getSecondaryTilesetPalettes()
        // for (let i = 0; i < secondaryPalettes.length; i++)
        //     applyTint(secondaryPalettes[i], tint)

        // map.setPrimaryTilesetPalettesPreview(primaryPalettes)
        // map.setSecondaryTilesetPalettesPreview(secondaryPalettes)
        map.setSecondaryTileset("gTileset_Rustboro")
    } catch(err) {
        console.log(err)
    }
}

// Porymap callback when project is opened.
export function on_project_opened(projectPath) {
    try {
        console.log(`opened ${projectPath}`)
        map.registerAction("resetTint", "View Day Tint")
        map.registerAction("applyMorningTint", "View Morning Tint")
        map.registerAction("applyNightTint", "View Night Tint")
    } catch(err) {
        console.log(err)
    }
}

export function resetTint() {
    applyTintToPalettes(normalTint)
}

export function applyMorningTint() {
    applyTintToPalettes(morningTint)
}

export function applyNightTint() {
    applyTintToPalettes(nightTint)
}
