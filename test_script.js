function randInt(min, max) {
    min = Math.ceil(min);
    max = Math.floor(max);
    return Math.floor(Math.random() * (max - min)) + min;
}

const grassTiles = [0x8, 0x9, 0x10, 0x11];

// Porymap callback when a block is painted.
export function on_block_changed(x, y, prevBlock, newBlock) {
    if (grassTiles.indexOf(newBlock.tile) != -1) {
        const i = randInt(0, grassTiles.length);
        api.scriptapi_setBlock(x, y, grassTiles[i], newBlock.collision, newBlock.elevation);
    }
}
