// dom.ts
function N(v) {
  if (typeof v === "string") {
    return document.createTextNode(v);
  }
  return v;
}
function addChildren(n, children) {
  if (Array.isArray(children)) {
    for (const child of children) {
      if (child) {
        n.appendChild(N(child));
      }
    }
  } else {
    if (children) {
      n.appendChild(N(children));
    }
  }
}
function E(type, classes, children) {
  const el = document.createElement(type);
  if (classes && classes.length > 0) {
    const actualClasses = classes.filter((c) => !!c);
    el.classList.add(...actualClasses);
  }
  if (children) {
    addChildren(el, children);
  }
  return el;
}
function F(children) {
  const f = document.createDocumentFragment();
  addChildren(f, children);
  return f;
}

// parser.ts
var LLMV_EOF = 255;
var LLMV_START = 1;
var LLMV_END = 2;
var LLMV_FIELD = 3;
var Parser = class {
  cur;
  buf;
  constructor(buf) {
    this.cur = 0;
    this.buf = buf;
  }
  parse() {
    const cRegions = [];
    parse:
      while (this.assertInBounds()) {
        switch (this.peekByte()) {
          case LLMV_EOF:
            break parse;
          case LLMV_START:
            {
              this.consumeByte(LLMV_START);
              const cRegion = {
                kind: this.consumeString(),
                addr: this.consume64(),
                size: this.consume64(),
                fields: []
              };
              fields:
                while (this.assertInBounds()) {
                  switch (this.peekByte()) {
                    case LLMV_FIELD:
                      {
                        this.consumeByte(LLMV_FIELD);
                        const field = {
                          name: this.consumeString(),
                          type: this.consumeString(),
                          addr: this.consume64(),
                          size: this.consume64()
                        };
                        cRegion.fields.push(field);
                      }
                      break;
                    case LLMV_END:
                      this.consumeByte(LLMV_END);
                      break fields;
                    default:
                      throw new Error(`Unexpected LLMV flag ${this.peekByte()} in region`);
                  }
                }
              cRegions.push(cRegion);
            }
            break;
          default:
            throw new Error(`Unexpected LLMV flag ${this.peekByte()}`);
        }
      }
    return cRegions;
  }
  peekByte() {
    return this.buf[this.cur];
  }
  consumeByte(b) {
    if (b !== void 0 && this.peekByte() !== b) {
      throw new Error(`expected ${b} but got ${this.peekByte()}`);
    }
    this.cur += 1;
  }
  consumeString() {
    let s = "";
    while (this.buf[this.cur] !== 0) {
      s += String.fromCharCode(this.buf[this.cur]);
      this.cur += 1;
    }
    this.cur += 1;
    return s;
  }
  consume64() {
    this.checkRemaining(8);
    const result = this.buf[this.cur + 7] << 56 | this.buf[this.cur + 6] << 48 | this.buf[this.cur + 5] << 40 | this.buf[this.cur + 4] << 32 | this.buf[this.cur + 3] << 24 | this.buf[this.cur + 2] << 16 | this.buf[this.cur + 1] << 8 | this.buf[this.cur + 0] << 0;
    this.cur += 8;
    return result;
  }
  checkRemaining(n) {
    if (this.buf.length - this.cur < n) {
      throw new Error(`fewer than ${n} bytes remaining in buffer`);
    }
  }
  assertInBounds() {
    if (this.cur >= this.buf.length) {
      throw new Error("ran out of buffer");
    }
    return true;
  }
};

// llmv.ts
var LLMV = class {
  /**
   * px per byte
   */
  zoom;
  constructor() {
    this.zoom = 24;
  }
  renderTape(tape) {
    let maxBars = 1;
    for (const region of tape.regions) {
      if (region.bars && region.bars.length > maxBars) {
        maxBars = region.bars.length;
      }
    }
    const elTape = E("div", ["llmv-tape"]);
    for (const region of tape.regions) {
      const elRegion = E("div", ["llmv-region"], [
        // region address
        E("div", ["llmv-code", "llmv-f3", "llmv-c2", "llmv-flex", "llmv-flex-column", "llmv-justify-end", "llmv-pl1", "llmv-pb1"], [
          Hex(region.addr)
        ])
      ]);
      const elFields = E("div", ["llmv-region-fields"]);
      for (const field of this.pad(region.addr, region.size, region.fields)) {
        const elField = E("div", ["llmv-field", "llmv-flex", "llmv-flex-column", "llmv-tc"]);
        elField.style.width = this.width(field.size);
        if (Array.isArray(field.content)) {
          const elSubfields = E("div", ["llmv-flex"]);
          for (const subfield of this.pad(field.addr, field.size, field.content)) {
            if (Array.isArray(subfield.content)) {
              throw new Error("can't have sub-sub-fields");
            }
            elSubfields.appendChild(FieldContent(subfield.content, "llmv-subfield"));
          }
          elField.appendChild(elSubfields);
        } else {
          elField.appendChild(FieldContent(field.content));
        }
        if (field.name) {
          let name = field.name;
          if (typeof name === "string") {
            name = name.trim() || "\xA0";
          }
          elField.appendChild(E("div", ["llmv-bt", "llmv-b2", "llmv-c2", "llmv-pa1", "llmv-f3"], name));
        }
        elFields.appendChild(elField);
      }
      elRegion.appendChild(elFields);
      const bars = [...region.bars ?? []];
      while (bars.length < maxBars) {
        bars.push({ addr: 0, size: 0 });
      }
      const elBars = E("div", ["llmv-flex", "llmv-flex-column"], bars.map((bar) => {
        const elBar = E("div", ["llmv-bar"]);
        elBar.style.marginLeft = this.width(bar.addr - region.addr);
        elBar.style.width = this.width(bar.size);
        if (bar.color) {
          elBar.style.backgroundColor = bar.color;
        }
        return elBar;
      }));
      elRegion.appendChild(elBars);
      elRegion.appendChild(E("div", ["llmv-f3", "llmv-tc"], region.description));
      elTape.appendChild(elRegion);
    }
    return elTape;
  }
  width(size) {
    return `${size * this.zoom}px`;
  }
  pad(baseAddr, size, fields) {
    const res = [];
    let lastAddr = baseAddr;
    for (const field of fields) {
      if (lastAddr < field.addr) {
        res.push({
          addr: lastAddr,
          size: field.addr - lastAddr,
          content: Padding()
        });
      }
      res.push(field);
      lastAddr = field.addr + field.size;
    }
    if (lastAddr < baseAddr + size) {
      res.push({
        addr: lastAddr,
        size: baseAddr + size - lastAddr,
        content: Padding()
      });
    }
    return res;
  }
};
function Padding() {
  return E("div", ["llmv-flex-grow-1", "llmv-striped"]);
}
function FieldContent(content, klass) {
  const classes = [klass, "llmv-flex-grow-1", "llmv-flex", "llmv-flex-column", "llmv-code", "llmv-f2"];
  if (typeof content === "string") {
    classes.push("llmv-pa1");
    return E("div", classes, [
      content
    ]);
  }
  return E("div", classes, content);
}
function Byte(n) {
  return Hex(n, false);
}
function Hex(n, prefix = true) {
  return `${prefix ? "0x" : ""}${n.toString(16)}`;
}
export {
  Byte,
  E,
  F,
  Hex,
  LLMV,
  N,
  Padding,
  Parser,
  addChildren
};
