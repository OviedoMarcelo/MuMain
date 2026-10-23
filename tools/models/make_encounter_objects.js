// Generates the models of the objects of the boss encounters: a floating crystal, a runic pillar and an
// obelisk, each in ice and fire colors. They are monsters of their own (see docs/custom-monsters.md),
// with a stone part and a glowing part, which the client draws additively (BlendMesh 1).
//
//   node tools/models/make_encounter_objects.js [extra output directory]
//
// Writes Data/Monster/Monster220.bmd to Monster225.bmd and their textures into src/bin/Data/Monster,
// and copies them to the extra directory too, if one is given. Needs Node.js, nothing else.
"use strict";

const fs = require("fs");
const path = require("path");

const MonsterDirectory = path.join(__dirname, "..", "..", "src", "bin", "Data", "Monster");

// ---------------------------------------------------------------------------------------------
// Vectors

const add = (a, b) => [a[0] + b[0], a[1] + b[1], a[2] + b[2]];
const sub = (a, b) => [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
const scale = (a, s) => [a[0] * s, a[1] * s, a[2] * s];
const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
const cross = (a, b) => [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
const normalize = a => scale(a, 1 / (Math.hypot(a[0], a[1], a[2]) || 1));

// Turns around the X axis (tilt), then around the Z axis (heading), then moves.
function placement(tilt, heading, offset) {
    return p => {
        const y1 = p[1] * Math.cos(tilt) - p[2] * Math.sin(tilt);
        const z1 = p[1] * Math.sin(tilt) + p[2] * Math.cos(tilt);
        const x2 = p[0] * Math.cos(heading) - y1 * Math.sin(heading);
        const y2 = p[0] * Math.sin(heading) + y1 * Math.cos(heading);
        return [x2 + offset[0], y2 + offset[1], z1 + offset[2]];
    };
}

const identity = p => p;

// ---------------------------------------------------------------------------------------------
// Meshes

class Mesh {
    constructor(textureName) {
        this.textureName = textureName;
        this.vertices = [];
        this.normals = [];
        this.texCoords = [];
        this.triangles = [];
    }

    // A flat triangle of the bone. It's turned so that it faces away from the inside point, as the
    // client expects counterclockwise triangles seen from the outside.
    triangle(node, points, uvs, inside) {
        let [a, b, c] = points;
        let [ua, ub, uc] = uvs;
        let normal = normalize(cross(sub(b, a), sub(c, a)));
        const centroid = scale(add(add(a, b), c), 1 / 3);
        if (dot(normal, sub(centroid, inside)) < 0) {
            [b, c] = [c, b];
            [ub, uc] = [uc, ub];
            normal = scale(normal, -1);
        }

        const first = this.vertices.length;
        const normalIndex = this.normals.length;
        for (const p of [a, b, c]) {
            this.vertices.push({ node, position: p });
        }

        for (const uv of [ua, ub, uc]) {
            this.texCoords.push(uv);
        }

        this.normals.push({ node, normal, bindVertex: first });
        this.triangles.push({ vertices: [first, first + 1, first + 2], normal: normalIndex, texCoords: [first, first + 1, first + 2] });
    }

    quad(node, points, uvs, inside) {
        this.triangle(node, [points[0], points[1], points[2]], [uvs[0], uvs[1], uvs[2]], inside);
        this.triangle(node, [points[0], points[2], points[3]], [uvs[0], uvs[2], uvs[3]], inside);
    }
}

function ring(sides, radius, z, angleOffset) {
    const points = [];
    for (let i = 0; i <= sides; i++) {
        const angle = angleOffset + (i / sides) * 2 * Math.PI;
        points.push([radius * Math.cos(angle), radius * Math.sin(angle), z]);
    }

    return points;
}

// The side of a prism or frustum from the ring at z0 to the one at z1. The texture goes
// uRepeat times around, from v0 at the bottom to v1 at the top.
function side(mesh, node, shape, sides, r0, z0, r1, z1, uv, angleOffset = 0) {
    const bottom = ring(sides, r0, z0, angleOffset);
    const top = ring(sides, r1, z1, angleOffset);
    const inside = shape([0, 0, (z0 + z1) / 2]);
    for (let i = 0; i < sides; i++) {
        const step = (uv.u1 - uv.u0) * uv.repeat / sides;
        const u0 = uv.u0 + step * i;
        const u1 = u0 + step;
        mesh.quad(node, [bottom[i], bottom[i + 1], top[i + 1], top[i]].map(shape),
            [[u0, uv.v0], [u1, uv.v0], [u1, uv.v1], [u0, uv.v1]], inside);
    }
}

// How much of the texture a cap shows around its center.
const CapUvRadius = 0.4;

// A flat cap of a ring; the inside point is below it for a top cap and above it for a bottom one.
function cap(mesh, node, shape, sides, radius, z, isTop, uv, angleOffset = 0) {
    const points = ring(sides, radius, z, angleOffset);
    const center = [0, 0, z];
    const inside = shape([0, 0, z + (isTop ? -1 : 1)]);
    for (let i = 0; i < sides; i++) {
        const around = k => [uv.u + CapUvRadius * Math.cos(k * 2 * Math.PI / sides), uv.v + CapUvRadius * Math.sin(k * 2 * Math.PI / sides)];
        mesh.triangle(node, [center, points[i], points[i + 1]].map(shape), [[uv.u, uv.v], around(i), around(i + 1)], inside);
    }
}

// A cone from a ring to a tip, for the ends of crystals and the top of the obelisk.
function cone(mesh, node, shape, sides, radius, z, tipZ, uv, angleOffset = 0) {
    const points = ring(sides, radius, z, angleOffset);
    const tip = [0, 0, tipZ];
    const inside = shape([0, 0, z]);
    for (let i = 0; i < sides; i++) {
        const u0 = uv.u0 + (uv.u1 - uv.u0) * i / sides;
        const u1 = uv.u0 + (uv.u1 - uv.u0) * (i + 1) / sides;
        mesh.triangle(node, [points[i], points[i + 1], tip].map(shape),
            [[u0, uv.vBase], [u1, uv.vBase], [(u0 + u1) / 2, uv.vTip]], inside);
    }
}

// A crystal: a prism with a pointed end at both sides, around its own center.
function crystal(mesh, node, shape, sides, radius, halfPrism, tip, uv) {
    side(mesh, node, shape, sides, radius, -halfPrism, radius, halfPrism, { u0: uv.u0, u1: uv.u1, v0: uv.v1, v1: uv.v0, repeat: 1 });
    cone(mesh, node, shape, sides, radius, halfPrism, halfPrism + tip, { u0: uv.u0, u1: uv.u1, vBase: uv.v0, vTip: uv.vTip });
    cone(mesh, node, shape, sides, radius, -halfPrism, -halfPrism - tip, { u0: uv.u0, u1: uv.u1, vBase: uv.v1, vTip: uv.vTip });
}

// ---------------------------------------------------------------------------------------------
// The atlas of the glowing parts: runes on the top half, the crystal on the bottom half.

const StoneUv = { u0: 0, u1: 1, v0: 1, v1: 0 };
const RuneUv = { u0: 0, u1: 1, v0: 0.48, v1: 0.02 };
const CrystalUv = { u0: 0, u1: 1, v0: 0.52, v1: 0.98, vTip: 0.75 };

// The bone of the parts which float above the object.
const RootNode = 0;
const FloatingNode = 1;

function createCrystalModel(textures) {
    const stone = new Mesh(textures.stone);
    const glow = new Mesh(textures.glow);

    // A low rocky pedestal on the ground.
    side(stone, RootNode, identity, 6, 72, 0, 56, 26, { ...StoneUv, repeat: 2 });
    cap(stone, RootNode, identity, 6, 56, 26, true, { u: 0.5, v: 0.5 });

    // A big crystal with four small ones around it, which float and turn together.
    crystal(glow, FloatingNode, identity, 6, 30, 45, 42, CrystalUv);
    for (let i = 0; i < 4; i++) {
        const heading = i * Math.PI / 2 + Math.PI / 4;
        const height = i % 2 === 0 ? 18 : -14;
        const shape = placement(0.45, heading, [58 * Math.cos(heading), 58 * Math.sin(heading), height]);
        crystal(glow, FloatingNode, shape, 6, 12, 18, 18, CrystalUv);
    }

    return { name: "EncounterCrystal", meshes: [stone, glow], floatingHeight: 175, totalHeight: 260 };
}

function createPillarModel(textures) {
    const stone = new Mesh(textures.stone);
    const glow = new Mesh(textures.glow);
    const offset = Math.PI / 8;

    // The plinth, the shaft and the capital of an octagonal column.
    side(stone, RootNode, identity, 8, 62, 0, 52, 30, { ...StoneUv, repeat: 2 }, offset);
    cap(stone, RootNode, identity, 8, 52, 30, true, { u: 0.5, v: 0.5 }, offset);
    side(stone, RootNode, identity, 8, 42, 30, 42, 330, { u0: 0, u1: 1, v0: 1, v1: -2, repeat: 2 }, offset);
    cap(stone, RootNode, identity, 8, 47, 330, false, { u: 0.5, v: 0.5 }, offset);
    side(stone, RootNode, identity, 8, 47, 330, 58, 362, { ...StoneUv, repeat: 2 }, offset);
    cap(stone, RootNode, identity, 8, 58, 362, true, { u: 0.5, v: 0.5 }, offset);

    // A band of runes around the shaft, a bit out of the stone.
    side(glow, RootNode, identity, 8, 43.5, 150, 43.5, 250, { ...RuneUv, repeat: 2 }, offset);

    // A crystal with three shards, which floats above the capital and turns.
    crystal(glow, FloatingNode, identity, 6, 18, 20, 40, CrystalUv);
    for (let i = 0; i < 3; i++) {
        const heading = i * 2 * Math.PI / 3;
        const shape = placement(0.6, heading, [36 * Math.cos(heading), 36 * Math.sin(heading), -8]);
        crystal(glow, FloatingNode, shape, 4, 6, 6, 12, CrystalUv);
    }

    return { name: "EncounterPillar", meshes: [stone, glow], floatingHeight: 440, totalHeight: 500 };
}

// How much of the width of a face of the obelisk its strip of runes takes, from its middle to each side.
const RuneStripWidth = 0.75;

function createObeliskModel(textures) {
    const stone = new Mesh(textures.stone);
    const glow = new Mesh(textures.glow);
    const offset = Math.PI / 4;
    const spireBottom = 40;
    const spireTop = 470;
    const bottomRadius = 46;
    const topRadius = 18;

    // A square base, a tall tapered spire and a pointed top.
    side(stone, RootNode, identity, 4, 85, 0, 74, spireBottom, { ...StoneUv, repeat: 1 }, offset);
    cap(stone, RootNode, identity, 4, 74, spireBottom, true, { u: 0.5, v: 0.5 }, offset);
    side(stone, RootNode, identity, 4, bottomRadius, spireBottom, topRadius, spireTop, { u0: 0, u1: 1, v0: 1, v1: -3, repeat: 1 }, offset);
    cone(stone, RootNode, identity, 4, topRadius, spireTop, 540, { u0: 0, u1: 1, vBase: 0.4, vTip: 0 }, offset);

    // A strip of runes in the middle of each face, a bit out of the stone. The runes go along the face.
    const radiusAt = z => bottomRadius + (topRadius - bottomRadius) * (z - spireBottom) / (spireTop - spireBottom);
    for (let face = 0; face < 4; face++) {
        const heading = face * Math.PI / 2;
        const outward = [Math.cos(heading), Math.sin(heading), 0];
        const along = [-Math.sin(heading), Math.cos(heading), 0];
        const corner = z => radiusAt(z) * Math.SQRT1_2;
        const point = (z, side) => add(add(scale(outward, corner(z) + 1.5), scale(along, side * corner(z) * RuneStripWidth)), [0, 0, z]);
        const low = 110;
        const high = 400;
        glow.quad(RootNode, [point(low, -1), point(low, 1), point(high, 1), point(high, -1)],
            [[RuneUv.u0, RuneUv.v1], [RuneUv.u0, RuneUv.v0], [RuneUv.u1, RuneUv.v0], [RuneUv.u1, RuneUv.v1]], [0, 0, (low + high) / 2]);
    }

    // A crystal which floats above the top and turns.
    crystal(glow, FloatingNode, identity, 4, 20, 8, 40, CrystalUv);

    return { name: "EncounterObelisk", meshes: [stone, glow], floatingHeight: 620, totalHeight: 670 };
}

// ---------------------------------------------------------------------------------------------
// Animations: the actions of a monster (stand 1 and 2, walk, attack 1 and 2, hit, die).

const IdleKeys = 40;
const HitKeys = 8;
const DieKeys = 16;
const BobHeight = 10;

function idle(model) {
    const root = [];
    const floating = [];
    for (let i = 0; i < IdleKeys; i++) {
        const turn = (i / IdleKeys) * 2 * Math.PI;
        root.push({ position: [0, 0, 0], rotation: [0, 0, 0] });
        floating.push({ position: [0, 0, model.floatingHeight + BobHeight * Math.sin(turn)], rotation: [0, 0, turn] });
    }

    return { root, floating };
}

// It shakes when it's hit, and the floating part a bit more.
function hit(model) {
    const root = [];
    const floating = [];
    for (let i = 0; i < HitKeys; i++) {
        const strength = 1 - i / HitKeys;
        const shake = (i % 2 === 0 ? 1 : -1) * strength;
        root.push({ position: [3 * shake, -2 * shake, 0], rotation: [0, 0, 0] });
        floating.push({ position: [8 * shake, 6 * shake, model.floatingHeight], rotation: [0.08 * shake, 0, 0] });
    }

    return { root, floating };
}

// It sinks into the ground leaning to a side, while the floating part falls and spins.
function die(model) {
    const root = [];
    const floating = [];
    for (let i = 0; i < DieKeys; i++) {
        const progress = i / (DieKeys - 1);
        const eased = progress * progress;
        root.push({ position: [0, 0, -model.totalHeight * 0.85 * eased], rotation: [0.25 * eased, 0, 0] });
        floating.push({ position: [0, 0, model.floatingHeight * (1 - 0.6 * eased)], rotation: [0.6 * eased, 0, 3 * Math.PI * eased] });
    }

    return { root, floating };
}

function animations(model) {
    const standing = idle(model);
    return [standing, standing, standing, standing, standing, hit(model), die(model)];
}

// ---------------------------------------------------------------------------------------------
// BMD (version 10, like the models of the client, see BMD::Open2)

const VertexSize = 16;
const NormalSize = 20;
const TexCoordSize = 8;
const TriangleSize = 64;
const NameSize = 32;

class Writer {
    constructor() {
        this.buffer = Buffer.alloc(1 << 22);
        this.offset = 0;
    }

    int16(value) { this.buffer.writeInt16LE(value, this.offset); this.offset += 2; }
    byte(value) { this.buffer.writeUInt8(value, this.offset); this.offset += 1; }
    float(value) { this.buffer.writeFloatLE(value, this.offset); this.offset += 4; }
    name(text) { this.buffer.write(text.slice(0, NameSize - 1), this.offset, "latin1"); this.offset += NameSize; }
    vector(v) { v.forEach(x => this.float(x)); }
    skip(bytes) { this.offset += bytes; }
}

function writeMesh(w, mesh, index) {
    w.int16(mesh.vertices.length);
    w.int16(mesh.normals.length);
    w.int16(mesh.texCoords.length);
    w.int16(mesh.triangles.length);
    w.int16(index);
    for (const vertex of mesh.vertices) {
        const start = w.offset;
        w.int16(vertex.node);
        w.skip(2);
        w.vector(vertex.position);
        w.offset = start + VertexSize;
    }

    for (const normal of mesh.normals) {
        const start = w.offset;
        w.int16(normal.node);
        w.skip(2);
        w.vector(normal.normal);
        w.int16(normal.bindVertex);
        w.offset = start + NormalSize;
    }

    for (const [u, v] of mesh.texCoords) {
        w.float(u);
        w.float(v);
    }

    for (const triangle of mesh.triangles) {
        const start = w.offset;
        w.byte(3);
        w.skip(1);
        [0, 1, 2, 0].forEach(k => w.int16(k < 3 ? triangle.vertices[k] : 0));
        [0, 0, 0, 0].forEach((_, k) => w.int16(k < 3 ? triangle.normal : 0));
        [0, 1, 2, 0].forEach(k => w.int16(k < 3 ? triangle.texCoords[k] : 0));
        w.offset = start + TriangleSize;
    }

    w.name(mesh.textureName);
}

function writeBone(w, name, parent, frames) {
    w.byte(0);
    w.name(name);
    w.int16(parent);
    for (const keys of frames) {
        keys.forEach(key => w.vector(key.position));
        keys.forEach(key => w.vector(key.rotation));
    }
}

function writeModel(model) {
    const actions = animations(model);
    const w = new Writer();
    w.buffer.write("BMD", 0, "latin1");
    w.offset = 3;
    w.byte(10);
    w.name(`${model.name}.smd`);
    w.int16(model.meshes.length);
    w.int16(2);
    w.int16(actions.length);
    model.meshes.forEach((mesh, index) => writeMesh(w, mesh, index));
    for (const action of actions) {
        w.int16(action.root.length);
        w.byte(0);
    }

    writeBone(w, "Root", -1, actions.map(a => a.root));
    writeBone(w, "Floating", 0, actions.map(a => a.floating));
    return w.buffer.subarray(0, w.offset);
}

// ---------------------------------------------------------------------------------------------
// Textures (OZT: a 32-bit uncompressed TGA behind a 4-byte header, with its rows from the bottom up)

const TextureSize = 256;

function random(seed) {
    let state = seed >>> 0;
    return () => {
        state = (state * 1664525 + 1013904223) >>> 0;
        return state / 4294967296;
    };
}

class Image {
    constructor() {
        this.pixels = new Float32Array(TextureSize * TextureSize * 3);
    }

    get(x, y) {
        const i = (y * TextureSize + x) * 3;
        return [this.pixels[i], this.pixels[i + 1], this.pixels[i + 2]];
    }

    set(x, y, color) {
        if (x < 0 || y < 0 || x >= TextureSize || y >= TextureSize) {
            return;
        }

        const i = (y * TextureSize + x) * 3;
        this.pixels[i] = color[0];
        this.pixels[i + 1] = color[1];
        this.pixels[i + 2] = color[2];
    }

    addAt(x, y, color, amount) {
        const xx = ((Math.round(x) % TextureSize) + TextureSize) % TextureSize;
        if (y < 0 || y >= TextureSize) {
            return;
        }

        const current = this.get(xx, Math.round(y));
        this.set(xx, Math.round(y), current.map((c, k) => c + color[k] * amount));
    }

    toOzt() {
        const header = Buffer.from([0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 8]);
        header.writeUInt16LE(TextureSize, 16);
        header.writeUInt16LE(TextureSize, 18);
        const body = Buffer.alloc(TextureSize * TextureSize * 4);
        let o = 0;
        for (let y = TextureSize - 1; y >= 0; y--) {
            for (let x = 0; x < TextureSize; x++) {
                const [r, g, b] = this.get(x, y).map(c => Math.max(0, Math.min(255, Math.round(c))));
                body[o++] = b;
                body[o++] = g;
                body[o++] = r;
                body[o++] = 255;
            }
        }

        return Buffer.concat([header, body]);
    }
}

// Smooth noise which repeats across the texture, for the stone.
function noise(seed, cells) {
    const next = random(seed);
    const grid = [];
    for (let i = 0; i < cells * cells; i++) {
        grid.push(next());
    }

    const at = (x, y) => grid[((y % cells) + cells) % cells * cells + ((x % cells) + cells) % cells];
    const smooth = t => t * t * (3 - 2 * t);
    return (x, y) => {
        const fx = x / TextureSize * cells;
        const fy = y / TextureSize * cells;
        const ix = Math.floor(fx);
        const iy = Math.floor(fy);
        const tx = smooth(fx - ix);
        const ty = smooth(fy - iy);
        const top = at(ix, iy) * (1 - tx) + at(ix + 1, iy) * tx;
        const bottom = at(ix, iy + 1) * (1 - tx) + at(ix + 1, iy + 1) * tx;
        return top * (1 - ty) + bottom * ty;
    };
}

// Gray stone in blocks, with a hint of the color of the object.
function createStoneTexture(tint) {
    const image = new Image();
    const layers = [noise(11, 4), noise(23, 8), noise(37, 16), noise(53, 32)];
    const blockHeight = 64;
    for (let y = 0; y < TextureSize; y++) {
        const row = Math.floor(y / blockHeight);
        const shift = row % 2 === 0 ? 0 : TextureSize / 4;
        for (let x = 0; x < TextureSize; x++) {
            const grain = layers.reduce((sum, layer, k) => sum + layer(x, y) / (1 << k), 0) / 1.875;
            const isMortar = y % blockHeight < 3 || (x + shift) % (TextureSize / 2) < 3;
            const light = (isMortar ? 0.45 : 0.75 + 0.5 * grain) * 92;
            image.set(x, y, [light * tint[0], light * tint[1], light * tint[2]]);
        }
    }

    return image;
}

// Draws a soft line, brighter in its middle.
function line(image, from, to, color, width) {
    const steps = Math.ceil(Math.hypot(to[0] - from[0], to[1] - from[1]) * 2);
    for (let s = 0; s <= steps; s++) {
        const x = from[0] + (to[0] - from[0]) * s / steps;
        const y = from[1] + (to[1] - from[1]) * s / steps;
        for (let dy = -width; dy <= width; dy++) {
            for (let dx = -width; dx <= width; dx++) {
                const distance = Math.hypot(dx, dy) / width;
                if (distance <= 1) {
                    image.addAt(x + dx, y + dy, color, 0.12 * (1 - distance));
                }
            }
        }
    }
}

// Glowing runes on black (which adds nothing when it's drawn additively), and a glowing crystal below them.
function createGlowTexture(colors, seed) {
    const image = new Image();
    const next = random(seed);
    const runeCount = 8;
    const runeWidth = TextureSize / runeCount;
    const half = TextureSize / 2;

    // Thin borders above and below the runes.
    for (let x = 0; x < TextureSize; x++) {
        for (const y of [10, half - 12]) {
            image.addAt(x, y, colors.rune, 0.9);
            image.addAt(x, y + 1, colors.rune, 0.5);
        }
    }

    // Each rune is a few strokes between the points of a small grid.
    for (let r = 0; r < runeCount; r++) {
        const left = r * runeWidth + runeWidth * 0.25;
        const grid = [];
        for (let gy = 0; gy < 4; gy++) {
            for (let gx = 0; gx < 3; gx++) {
                grid.push([left + gx * runeWidth * 0.25, 28 + gy * 22]);
            }
        }

        const strokes = 3 + Math.floor(next() * 3);
        for (let s = 0; s < strokes; s++) {
            const a = grid[Math.floor(next() * grid.length)];
            const b = grid[Math.floor(next() * grid.length)];
            line(image, a, b, colors.rune, 3);
            line(image, a, b, colors.core, 1);
        }
    }

    paintCrystal(image, colors, next);
    return image;
}

// The faces of a crystal (as many as the sides of the crystals of the models, which go around the texture once).
const CrystalFaces = 6;
const CrackCount = 14;
const SparkleCount = 40;

// The crystal on the bottom half: every face with its own brightness and shade, cloudy inside, with
// glowing cracks and a few sparkles, and brighter on the edges of the faces.
function paintCrystal(image, colors, next) {
    const half = TextureSize / 2;
    const faceWidth = TextureSize / CrystalFaces;
    const faces = Array.from({ length: CrystalFaces }, () => ({ brightness: 0.6 + 0.5 * next(), shade: next(), slant: next() - 0.5 }));
    const clouds = [noise(71, 4), noise(89, 8), noise(97, 16)];
    for (let y = half; y < TextureSize; y++) {
        const height = (y - half) / half;
        const middle = 1 - Math.abs(height - 0.5) * 2;
        for (let x = 0; x < TextureSize; x++) {
            const face = faces[Math.floor(x / faceWidth)];
            const across = (x % faceWidth) / faceWidth;
            const edge = Math.pow(Math.abs(across - 0.5) * 2, 6);
            const cloud = clouds.reduce((sum, layer, k) => sum + layer(x, y) / (1 << k), 0) / 1.75;
            // A band of light which crosses the face at its own slant.
            const band = Math.max(0, 1 - Math.abs(height - 0.5 - face.slant * (across - 0.5)) * 6);
            const shine = face.brightness * (0.25 + 0.35 * middle + 0.55 * cloud) + 0.5 * edge + 0.25 * band;
            const tone = colors.crystal.map((c, k) => c + (colors.crystalDeep[k] - c) * face.shade * (1 - cloud));
            image.set(x, y, tone.map((c, k) => c * shine + colors.core[k] * (0.35 * edge + 0.2 * band)));
        }
    }

    for (let i = 0; i < CrackCount; i++) {
        let point = [next() * TextureSize, half + 8 + next() * (half - 16)];
        const angle = next() * Math.PI * 2;
        const segments = 2 + Math.floor(next() * 3);
        for (let k = 0; k < segments; k++) {
            const turn = angle + (next() - 0.5) * 1.2;
            const length = 6 + next() * 14;
            const end = [point[0] + Math.cos(turn) * length, Math.min(TextureSize - 2, Math.max(half + 2, point[1] + Math.sin(turn) * length))];
            line(image, point, end, colors.core, 1);
            point = end;
        }
    }

    for (let i = 0; i < SparkleCount; i++) {
        const x = next() * TextureSize;
        const y = half + next() * half;
        image.addAt(x, y, colors.core, 1.2);
        for (const [dx, dy] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) {
            image.addAt(x + dx, y + dy, colors.core, 0.5);
        }
    }
}

// ---------------------------------------------------------------------------------------------

const Palettes = {
    Ice: { rune: [90, 200, 255], core: [235, 250, 255], crystal: [90, 185, 255], crystalDeep: [30, 70, 210], stoneTint: [0.92, 0.97, 1.08] },
    Fire: { rune: [255, 120, 30], core: [255, 235, 180], crystal: [255, 120, 30], crystalDeep: [200, 30, 15], stoneTint: [1.08, 0.94, 0.88] },
};

// The model types 219 to 224 of the client, whose files are Monster220 to Monster225.
const Models = [
    { file: "Monster220.bmd", create: createCrystalModel, palette: "Ice" },
    { file: "Monster221.bmd", create: createCrystalModel, palette: "Fire" },
    { file: "Monster222.bmd", create: createPillarModel, palette: "Ice" },
    { file: "Monster223.bmd", create: createPillarModel, palette: "Fire" },
    { file: "Monster224.bmd", create: createObeliskModel, palette: "Ice" },
    { file: "Monster225.bmd", create: createObeliskModel, palette: "Fire" },
];

function main() {
    const outputs = [MonsterDirectory, ...process.argv.slice(2)];
    const files = new Map();
    for (const [name, palette] of Object.entries(Palettes)) {
        files.set(`CyberEncounterStone${name}.ozt`, createStoneTexture(palette.stoneTint).toOzt());
        files.set(`CyberEncounterGlow${name}.ozt`, createGlowTexture(palette, name.length * 97).toOzt());
    }

    for (const model of Models) {
        const textures = { stone: `CyberEncounterStone${model.palette}.tga`, glow: `CyberEncounterGlow${model.palette}.tga` };
        files.set(model.file, writeModel(model.create(textures)));
    }

    for (const directory of outputs) {
        fs.mkdirSync(directory, { recursive: true });
        for (const [name, data] of files) {
            fs.writeFileSync(path.join(directory, name), data);
        }

        console.log(`${files.size} files written to ${directory}`);
    }
}

main();
