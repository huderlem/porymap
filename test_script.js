export function on_block_changed(x, y, prevBlock, newBlock) {
    try {
        console.log("on_block_changed", x, y)
    } catch (err) {
        console.log(err)
    }
}
