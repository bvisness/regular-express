import { Hex, LLMV, Parser } from "./llmv.js"

const llmv = new LLMV();

let pools = [];
let cRegions = {};

export function initViz() {
  pools = [
    regex.getPool_Group(),
    regex.getPool_NoUnionEx(),
    regex.getPool_Regex(),
    regex.getPool_Set(),
    regex.getPool_SetItem(),
    regex.getPool_Unit(),
  ];
}

function subMem(addr, size) {
  return new Uint8Array(mem.buffer).subarray(addr, addr + size);
}

export function viz(el) {
  el.innerHTML = "";

  cRegions = {};
  for (const pool of pools) {
    const err = regex.pool_viz(pool)
    if (err) {
      throw new Error("failed to visualize pool");
    }

    const buf = subMem(regex.vizbuf(), regex.vizbuf_size());
    const parser = new Parser(buf);
    const result = parser.parse();

    for (const cRegion of result) {
      saveCRegion(cRegion);
    }
  }

  // Render active stuff
  el.appendChild(Pool(regex.getPool_Regex()));
  el.appendChild(Pool(regex.getPool_Group()));
  el.appendChild(Pool(regex.getPool_Unit()));
}

function saveCRegion(cRegion) {
  if (!cRegions[cRegion.kind]) {
    cRegions[cRegion.kind] = {};
  }
  cRegions[cRegion.kind][cRegion.addr] = cRegion;
}

function getCRegion(kind, addr) {
  return cRegions[kind]?.[addr];
}

function must(v) {
  if (!v) {
    throw new Error(`expected a truthy value but got ${v}`);
  }
  return v;
}

function Pool(addr) {
  const pool = must(getCRegion("Pool", addr));
  const nameField = getCField(pool, "name");
  const name = must(getCRegion("cstring", littleEndian(subMem(nameField.addr, nameField.size))));

  const region = {
    addr: pool.addr,
    size: pool.size,
    fields: [],
    description: `Pool (${cstringToString(name)})`,
  };
  for (const f of pool.fields) {
    region.fields.push({
      addr: f.addr,
      size: f.size,
      name: f.name,
      content: defaultCFieldContent(f),
    });
  }

  return llmv.renderTape({
    regions: [region],
  });
}

function getCField(cRegion, name) {
  for (const f of cRegion.fields) {
    if (f.name === name) {
      return f;
    }
  }
  throw new Error(`no field named "${name}" on ${cRegion.kind}`);
}

function defaultCFieldContent(f) {
  const num = littleEndian(subMem(f.addr, f.size));

  if (f.type[f.type.length-1] === "*") {
    return Hex(num) + "*";
  }
  return Hex(num);
}

function littleEndian(bytes) {
  let result = 0n;
  for (let i = 0; i < bytes.length; i++) {
    result |= BigInt(bytes[i] << (i * 8));
  }
  return result;
}

function cstringToString(f) {
  if (f.kind !== "cstring") {
    throw new Error(`need a CField of kind "cstring", got ${f.kind}`);
  }

  let res = "";
  for (const b of subMem(f.addr, f.size)) {
    if (b === 0) {
      break;
    }
    res += String.fromCharCode(b);
  }
  return res;
}
