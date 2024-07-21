import { Hex, LLMV, Parser, E } from "./llmv.js"

const llmv = new LLMV();

let allPools = [];
let cRegions = {};

let openRegions = [];

export function initViz() {
  allPools = [
    regex.getPool_Group(),
    regex.getPool_NoUnionEx(),
    regex.getPool_Regex(),
    regex.getPool_Set(),
    regex.getPool_SetItem(),
    regex.getPool_Unit(),
  ];
  openRegions = [
    {
      kind: "Pool",
      addr: regex.getPool_Regex(),
      children: [],
    },
    {
      kind: "Pool",
      addr: regex.getPool_Group(),
      children: [],
    },
    {
      kind: "Pool",
      addr: regex.getPool_Unit(),
      children: [],
    },
  ]
}

function addOpenChild(openRegion, kind, addr) {
  openRegion.children.push({
    kind: kind,
    addr: addr,
    children: [],
  });
}

function subMem(addr, size) {
  return new Uint8Array(mem.buffer).subarray(addr, addr + size);
}

export function viz(el) {
  el.innerHTML = "";

  cRegions = {};
  for (const pool of allPools) {
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
  for (const r of openRegions) {
    el.appendChild(llmv.renderTape(vizOpenRegion(r)));
  }
}

function vizOpenRegion(r) {
  const cRegion = getCRegion(r.kind, r.addr);
  if (!cRegion) {
    return E("div", [], `No ${r.kind} found at address ${Hex(r.addr)}`);
  }

  let tape = null;
  switch (r.kind) {
    case "Pool":
      tape = Pool(r);
      break;
    case "cstring":
      tape = CString(r.addr);
      break;
    case "PoolFreeNode":
      tape = PoolFreeNode(r);
      break;
    default:
      console.warn("Unknown open region kind", r.kind);
      tape = {
        regions: [{
          addr: 0,
          size: 0,
          fields: [],
          description: "???",
        }],
      };
  }
  tape.children = r.children.map(c => vizOpenRegion(c));

  return tape;
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

function Pool(openRegion) {
  const pool = must(getCRegion("Pool", openRegion.addr));
  const name = must(getCRegion("cstring", ptrval(getCField(pool, "name"))));

  const region = CStruct(pool, {
    "name": {
      onclick() {
        addOpenChild(openRegion, "cstring", name.addr);
      },
    },
    "head": {
      onclick() {
        addOpenChild(openRegion, "PoolFreeNode", ptrval(getCField(pool, "head")));
      },
    },
  });
  region.description = `Pool (${cstringToString(name)})`;

  return {
    regions: [region],
  };
}

function PoolFreeNode(openRegion) {
  const node = must(getCRegion("PoolFreeNode", openRegion.addr));
  return {
    regions: [CStruct(node, {
      "next": {
        onclick() {
          addOpenChild(openRegion, "PoolFreeNode", ptrval(getCField(node, "next")));
        },
      }
    })],
  };
}

function CStruct(cRegion, fieldOverrides = {}) {
  return {
    addr: cRegion.addr,
    size: cRegion.size,
    fields: cRegion.fields.map(cf => {
      const override = fieldOverrides[cf.name] ?? {};
      return {
        addr: cf.addr,
        size: cf.size,
        name: override.name ?? cf.name,
        content: override.content ?? defaultCFieldContent(cf),
        onclick: override.onclick,
      };
    }),
    description: cRegion.kind,
  };
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
  const num = intval(f);

  if (f.size > 8) {
    return `(${f.type} data)`;
  }
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

function ptrval(f) {
  return intval(f);
}

function intval(f) {
  return littleEndian(subMem(f.addr, f.size));
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

function CString(addr) {
  const cstr = must(getCRegion("cstring", addr));
  const fields = [];
  const strMem = subMem(cstr.addr, cstr.size);
  for (let i = 0; i < cstr.size; i++) {
    fields.push({
      addr: cstr.addr + i,
      size: 1,
      name: String.fromCharCode(strMem[i]),
      content: Hex(strMem[i], false),
    });
  }
  return {
    regions: [{
      addr: cstr.addr,
      size: cstr.size,
      fields: fields,
      description: `"${cstringToString(cstr)}"`,
    }],
  };
}
