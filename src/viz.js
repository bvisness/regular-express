import { LLMV, Parser } from "./llmv.js"

const llmv = new LLMV();

export function viz(el) {
  el.innerHTML = ":o";
  const err = regex.pool_viz(regex.getPool_Regex())
  if (err) {
    throw new Error("failed to visualize pool");
  }

  const addr = regex.vizbuf();
  const size = regex.vizbuf_size();
  const buf = new Uint8Array(mem.buffer).subarray(addr, addr + size);

  const parser = new Parser(buf);
  const cRegions = parser.parse();

  console.log(cRegions);
}
