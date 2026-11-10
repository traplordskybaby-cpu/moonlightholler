import sys
from PIL import Image

class GB:
    def __init__(self, rom):
        self.rom = bytearray(rom)
        self.vram = [bytearray(0x2000), bytearray(0x2000)]
        self.vbk = 0
        self.wram = [bytearray(0x1000) for _ in range(8)]
        self.svbk = 1
        self.oam = bytearray(0xA0)
        self.io = bytearray(0x80)
        self.hram = bytearray(0x80)
        self.ie = 0
        self.R = [0x00, 0x00, 0xFF, 0x56, 0x00, 0x0D, 0, 0x11]  # B C D E H L (hl) A
        self.fz, self.fn, self.fh, self.fc = 1, 0, 0, 0
        self.sp = 0xFFFE
        self.pc = 0x100
        self.ime = False
        self.ei_pending = False
        self.halted = False
        self.cy = 0
        self.line_cy = 0
        self.ly = 0
        self.frames = 0
        self.joy = 0  # bits: right,left,up,down,a,b,select,start (1=pressed)
        self.io[0x40] = 0x91
        self.io[0x47] = 0xFC
        self.io[0x0F] = 0xE1
        self.bgpal = bytearray(64)
        self.obpal = bytearray(64)
        for i in range(64):
            self.bgpal[i] = 0xFF
            self.obpal[i] = 0xFF
        self.cgb_pal_used = False
        self.bcps = 0
        self.ocps = 0
        self.breakpoints = {}
        self.trace_writes = {}
        self.div_base = 0

    # ---------- memory ----------
    def rd(self, a):
        self.cy += 4
        return self.rd_(a)

    def rd_(self, a):
        if a < 0x8000:
            return self.rom[a]
        if a < 0xA000:
            return self.vram[self.vbk][a - 0x8000]
        if a < 0xC000:
            return 0xFF
        if a < 0xD000:
            return self.wram[0][a - 0xC000]
        if a < 0xE000:
            return self.wram[self.svbk][a - 0xD000]
        if a < 0xF000:
            return self.wram[0][a - 0xE000]
        if a < 0xFE00:
            return self.wram[self.svbk][a - 0xF000]
        if a < 0xFEA0:
            return self.oam[a - 0xFE00]
        if a < 0xFF00:
            return 0xFF
        if a < 0xFF80:
            return self.rdio(a - 0xFF00)
        if a < 0xFFFF:
            return self.hram[a - 0xFF80]
        return self.ie

    def rdio(self, r):
        if r == 0x00:
            sel = self.io[0]
            v = 0xCF
            btn = 0
            if not (sel & 0x10):  # dpad
                btn |= (~self.joy >> 0) & 0xF
                v &= ~0 & 0xFF
                return 0xC0 | (sel & 0x30) | ((~(self.joy & 0xF)) & 0xF)
            if not (sel & 0x20):
                return 0xC0 | (sel & 0x30) | ((~((self.joy >> 4) & 0xF)) & 0xF)
            return 0xFF
        if r == 0x04:
            return ((self.cy - self.div_base) >> 8) & 0xFF
        if r == 0x44:
            return self.ly
        if r == 0x4F:
            return 0xFE | self.vbk
        if r == 0x70:
            return self.svbk
        if r == 0x69:
            return self.bgpal[self.bcps & 0x3F]
        if r == 0x6B:
            return self.obpal[self.ocps & 0x3F]
        return self.io[r]

    def wr(self, a, v):
        self.cy += 4
        self.wr_(a, v)

    def wr_(self, a, v):
        v &= 0xFF
        if a in self.trace_writes:
            self.trace_writes[a].append((self.pc, v, self.frames))
        if a < 0x8000:
            return
        if a < 0xA000:
            self.vram[self.vbk][a - 0x8000] = v
        elif a < 0xC000:
            pass
        elif a < 0xD000:
            self.wram[0][a - 0xC000] = v
        elif a < 0xE000:
            self.wram[self.svbk][a - 0xD000] = v
        elif a < 0xF000:
            self.wram[0][a - 0xE000] = v
        elif a < 0xFE00:
            self.wram[self.svbk][a - 0xF000] = v
        elif a < 0xFEA0:
            self.oam[a - 0xFE00] = v
        elif a < 0xFF00:
            pass
        elif a < 0xFF80:
            self.wrio(a - 0xFF00, v)
        elif a < 0xFFFF:
            self.hram[a - 0xFF80] = v
        else:
            self.ie = v

    def wrio(self, r, v):
        if r == 0x04:
            self.div_base = self.cy
            return
        if r == 0x46:
            src = v << 8
            for i in range(0xA0):
                self.oam[i] = self.rd_(src + i)
            return
        if r == 0x4F:
            self.vbk = v & 1
            return
        if r == 0x70:
            self.svbk = (v & 7) or 1
            return
        if r == 0x68:
            self.bcps = v
            return
        if r == 0x69:
            self.cgb_pal_used = True
            self.bgpal[self.bcps & 0x3F] = v
            if self.bcps & 0x80:
                self.bcps = 0x80 | ((self.bcps + 1) & 0x3F)
            return
        if r == 0x6A:
            self.ocps = v
            return
        if r == 0x6B:
            self.cgb_pal_used = True
            self.obpal[self.ocps & 0x3F] = v
            if self.ocps & 0x80:
                self.ocps = 0x80 | ((self.ocps + 1) & 0x3F)
            return
        if r == 0x41:
            self.io[r] = v & 0x78
            return
        if r == 0x40:
            if not (v & 0x80):
                self.ly = 0
                self.line_cy = 0
            self.io[r] = v
            return
        self.io[r] = v

    def rd16(self, a):
        return self.rd(a) | (self.rd((a + 1) & 0xFFFF) << 8)

    def push(self, v):
        self.sp = (self.sp - 1) & 0xFFFF
        self.wr(self.sp, v >> 8)
        self.sp = (self.sp - 1) & 0xFFFF
        self.wr(self.sp, v & 0xFF)

    def pop(self):
        lo = self.rd(self.sp)
        self.sp = (self.sp + 1) & 0xFFFF
        hi = self.rd(self.sp)
        self.sp = (self.sp + 1) & 0xFFFF
        return lo | hi << 8

    def f8(self):
        v = self.rd(self.pc)
        self.pc = (self.pc + 1) & 0xFFFF
        return v

    def f16(self):
        lo = self.f8()
        hi = self.f8()
        return lo | hi << 8

    # ---------- registers ----------
    def get8(self, i):
        if i == 6:
            return self.rd(self.R[4] << 8 | self.R[5])
        return self.R[i]

    def set8(self, i, v):
        if i == 6:
            self.wr(self.R[4] << 8 | self.R[5], v)
        else:
            self.R[i] = v & 0xFF

    def getrr(self, i):
        if i == 0: return self.R[0] << 8 | self.R[1]
        if i == 1: return self.R[2] << 8 | self.R[3]
        if i == 2: return self.R[4] << 8 | self.R[5]
        return self.sp

    def setrr(self, i, v):
        v &= 0xFFFF
        if i == 0: self.R[0], self.R[1] = v >> 8, v & 0xFF
        elif i == 1: self.R[2], self.R[3] = v >> 8, v & 0xFF
        elif i == 2: self.R[4], self.R[5] = v >> 8, v & 0xFF
        else: self.sp = v

    def getaf(self):
        return self.R[7] << 8 | (self.fz << 7 | self.fn << 6 | self.fh << 5 | self.fc << 4)

    def setaf(self, v):
        self.R[7] = v >> 8
        self.fz = (v >> 7) & 1; self.fn = (v >> 6) & 1; self.fh = (v >> 5) & 1; self.fc = (v >> 4) & 1

    def cond(self, i):
        return [not self.fz, self.fz, not self.fc, self.fc][i]

    def alu(self, op, v):
        a = self.R[7]
        if op == 0:
            r = a + v; self.fh = int((a & 15) + (v & 15) > 15); self.fc = int(r > 255); self.fn = 0
        elif op == 1:
            c = self.fc; r = a + v + c; self.fh = int((a & 15) + (v & 15) + c > 15); self.fc = int(r > 255); self.fn = 0
        elif op == 2 or op == 7:
            r = a - v; self.fh = int((a & 15) < (v & 15)); self.fc = int(r < 0); self.fn = 1
        elif op == 3:
            c = self.fc; r = a - v - c; self.fh = int((a & 15) < (v & 15) + c); self.fc = int(r < 0); self.fn = 1
        elif op == 4:
            r = a & v; self.fh = 1; self.fc = 0; self.fn = 0
        elif op == 5:
            r = a ^ v; self.fh = 0; self.fc = 0; self.fn = 0
        else:
            r = a | v; self.fh = 0; self.fc = 0; self.fn = 0
        r &= 0xFF
        self.fz = int(r == 0)
        if op != 7:
            self.R[7] = r

    def cb(self, c):
        x = c >> 6; y = (c >> 3) & 7; z = c & 7
        v = self.get8(z)
        if x == 0:
            if y == 0:
                self.fc = v >> 7; v = ((v << 1) | self.fc) & 0xFF
            elif y == 1:
                self.fc = v & 1; v = (v >> 1) | (self.fc << 7)
            elif y == 2:
                c_ = self.fc; self.fc = v >> 7; v = ((v << 1) | c_) & 0xFF
            elif y == 3:
                c_ = self.fc; self.fc = v & 1; v = (v >> 1) | (c_ << 7)
            elif y == 4:
                self.fc = v >> 7; v = (v << 1) & 0xFF
            elif y == 5:
                self.fc = v & 1; v = (v >> 1) | (v & 0x80)
            elif y == 6:
                v = ((v << 4) | (v >> 4)) & 0xFF; self.fc = 0
            else:
                self.fc = v & 1; v >>= 1
            self.fz = int(v == 0); self.fn = 0; self.fh = 0
            self.set8(z, v)
        elif x == 1:
            self.fz = int(not (v >> y) & 1); self.fn = 0; self.fh = 1
        elif x == 2:
            self.set8(z, v & ~(1 << y))
        else:
            self.set8(z, v | (1 << y))

    # ---------- interrupts / timing ----------
    def tick_ppu(self, dcy):
        if not (self.io[0x40] & 0x80):
            return
        self.line_cy += dcy
        while self.line_cy >= 456:
            self.line_cy -= 456
            self.ly = (self.ly + 1) % 154
            if self.ly == 144:
                self.io[0x0F] |= 1
                self.frames += 1
                self.vblank_hook()

    def vblank_hook(self):
        pass

    def step(self):
        c0 = self.cy
        if self.ei_pending:
            self.ei_pending = False
            self.ime = True
        pend = self.io[0x0F] & self.ie & 0x1F
        if pend:
            self.halted = False
            if self.ime:
                self.ime = False
                for b in range(5):
                    if pend & (1 << b):
                        self.io[0x0F] &= ~(1 << b)
                        self.push(self.pc)
                        self.pc = 0x40 + 8 * b
                        self.cy += 8
                        break
                self.tick_ppu(self.cy - c0)
                return
        if self.halted:
            self.cy += 4
            self.tick_ppu(4)
            return
        self.exec1()
        self.tick_ppu(self.cy - c0)

    def exec1(self):
        pc0 = self.pc
        o = self.f8()
        if o == 0xCB:
            self.cb(self.f8()); return
        if o < 0x40:
            x = o & 7
            if o == 0x00: return
            if o == 0x10: self.f8(); return
            if o == 0x08:
                a = self.f16(); self.wr(a, self.sp & 0xFF); self.wr(a + 1, self.sp >> 8); return
            if o == 0x18:
                e = self.f8(); self.pc = (self.pc + (e - 256 if e > 127 else e)) & 0xFFFF; self.cy += 4; return
            if o in (0x20, 0x28, 0x30, 0x38):
                e = self.f8()
                if self.cond((o >> 3) & 3):
                    self.pc = (self.pc + (e - 256 if e > 127 else e)) & 0xFFFF; self.cy += 4
                return
            if x == 1:
                i = o >> 4
                if not (o & 8):
                    self.setrr(i, self.f16())
                else:
                    hl = self.getrr(2); v = self.getrr(i)
                    self.fn = 0; self.fh = int((hl & 0xFFF) + (v & 0xFFF) > 0xFFF); self.fc = int(hl + v > 0xFFFF)
                    self.setrr(2, hl + v); self.cy += 4
                return
            if x == 2:
                i = o >> 4
                hl = self.getrr(2)
                if i == 0: a = self.getrr(0)
                elif i == 1: a = self.getrr(1)
                else: a = hl
                if not (o & 8):
                    self.wr(a, self.R[7])
                else:
                    self.R[7] = self.rd(a)
                if i == 2: self.setrr(2, hl + 1)
                if i == 3: self.setrr(2, hl - 1)
                return
            if x == 3:
                i = o >> 4
                self.setrr(i, self.getrr(i) + (1 if not (o & 8) else -1)); self.cy += 4; return
            if x == 4:
                i = o >> 3; v = (self.get8(i) + 1) & 0xFF
                self.fz = int(v == 0); self.fn = 0; self.fh = int((v & 15) == 0); self.set8(i, v); return
            if x == 5:
                i = o >> 3; v = (self.get8(i) - 1) & 0xFF
                self.fz = int(v == 0); self.fn = 1; self.fh = int((v & 15) == 15); self.set8(i, v); return
            if x == 6:
                self.set8(o >> 3, self.f8()); return
            a = self.R[7]
            k = o >> 3 & 7
            if k == 0:
                self.fc = a >> 7; a = ((a << 1) | self.fc) & 0xFF; self.fz = self.fn = self.fh = 0
            elif k == 1:
                self.fc = a & 1; a = (a >> 1) | (self.fc << 7); self.fz = self.fn = self.fh = 0
            elif k == 2:
                c_ = self.fc; self.fc = a >> 7; a = ((a << 1) | c_) & 0xFF; self.fz = self.fn = self.fh = 0
            elif k == 3:
                c_ = self.fc; self.fc = a & 1; a = (a >> 1) | (c_ << 7); self.fz = self.fn = self.fh = 0
            elif k == 4:
                if not self.fn:
                    if self.fc or a > 0x99: a += 0x60; self.fc = 1
                    if self.fh or (a & 0xF) > 9: a += 6
                else:
                    if self.fc: a -= 0x60
                    if self.fh: a -= 6
                a &= 0xFF; self.fz = int(a == 0); self.fh = 0
            elif k == 5:
                a ^= 0xFF; self.fn = 1; self.fh = 1
            elif k == 6:
                self.fn = 0; self.fh = 0; self.fc = 1
            else:
                self.fn = 0; self.fh = 0; self.fc ^= 1
            self.R[7] = a
            return
        if o < 0x80:
            if o == 0x76:
                self.halted = True; return
            self.set8((o >> 3) & 7, self.get8(o & 7)); return
        if o < 0xC0:
            self.alu((o >> 3) & 7, self.get8(o & 7)); return
        x = o & 7
        if x == 0:
            if o in (0xC0, 0xC8, 0xD0, 0xD8):
                self.cy += 4
                if self.cond((o >> 3) & 3):
                    self.pc = self.pop(); self.cy += 4
                return
            if o == 0xE0: self.wr(0xFF00 + self.f8(), self.R[7]); return
            if o == 0xF0: self.R[7] = self.rd(0xFF00 + self.f8()); return
            if o in (0xE8, 0xF8):
                e = self.f8(); s = self.sp
                self.fz = 0; self.fn = 0; self.fh = int((s & 15) + (e & 15) > 15); self.fc = int((s & 255) + e > 255)
                r = (s + (e - 256 if e > 127 else e)) & 0xFFFF
                if o == 0xE8: self.sp = r; self.cy += 8
                else: self.setrr(2, r); self.cy += 4
                return
        if x == 1:
            if not (o & 8):
                i = (o >> 4) & 3
                v = self.pop()
                if i == 3: self.setaf(v)
                else: self.setrr(i, v)
                return
            if o == 0xC9: self.pc = self.pop(); self.cy += 4; return
            if o == 0xD9: self.pc = self.pop(); self.ime = True; self.cy += 4; return
            if o == 0xE9: self.pc = self.getrr(2); return
            if o == 0xF9: self.sp = self.getrr(2); self.cy += 4; return
        if x == 2:
            if o in (0xC2, 0xCA, 0xD2, 0xDA):
                a = self.f16()
                if self.cond((o >> 3) & 3): self.pc = a; self.cy += 4
                return
            if o == 0xE2: self.wr(0xFF00 + self.R[1], self.R[7]); return
            if o == 0xF2: self.R[7] = self.rd(0xFF00 + self.R[1]); return
            if o == 0xEA: self.wr(self.f16(), self.R[7]); return
            if o == 0xFA: self.R[7] = self.rd(self.f16()); return
        if x == 3:
            if o == 0xC3: self.pc = self.f16(); self.cy += 4; return
            if o == 0xF3: self.ime = False; return
            if o == 0xFB: self.ei_pending = True; return
        if x == 4 and o in (0xC4, 0xCC, 0xD4, 0xDC):
            a = self.f16()
            if self.cond((o >> 3) & 3):
                self.push(self.pc); self.pc = a; self.cy += 4
            return
        if x == 5:
            if not (o & 8):
                i = (o >> 4) & 3
                self.push(self.getaf() if i == 3 else self.getrr(i)); self.cy += 4; return
            if o == 0xCD:
                a = self.f16(); self.push(self.pc); self.pc = a; self.cy += 4; return
        if x == 6:
            self.alu((o >> 3) & 7, self.f8()); return
        if x == 7:
            self.push(self.pc); self.pc = o & 0x38; self.cy += 4; return
        raise Exception('bad opcode %02x at %04x' % (o, pc0))

    # ---------- rendering ----------
    def color(self, pal, idx):
        lo = pal[idx * 2]; hi = pal[idx * 2 + 1]
        v = lo | hi << 8
        r = (v & 31) << 3; g = ((v >> 5) & 31) << 3; b = ((v >> 10) & 31) << 3
        return (r | r >> 5, g | g >> 5, b | b >> 5)

    def render(self):
        lcdc = self.io[0x40]
        img = Image.new('RGB', (160, 144), (255, 255, 255))
        px = img.load()
        if not (lcdc & 0x80):
            return img
        bgp = self.io[0x47]
        grays = [(232, 232, 208), (168, 184, 120), (96, 120, 72), (24, 32, 24)]
        bgidx = [[0] * 160 for _ in range(144)]
        bgprio = [[False] * 160 for _ in range(144)]
        v0, v1 = self.vram
        cgb = self.cgb_pal_used

        def tilepix(bank, tile, x, y):
            vr = self.vram[bank]
            if lcdc & 0x10:
                base = tile * 16
            else:
                base = 0x1000 + ((tile if tile < 128 else tile - 256) * 16)
            lo = vr[base + y * 2]; hi = vr[base + y * 2 + 1]
            return ((hi >> (7 - x)) & 1) << 1 | ((lo >> (7 - x)) & 1)

        scx, scy = self.io[0x43], self.io[0x42]
        wx, wy = self.io[0x4B] - 7, self.io[0x4A]
        bgmap = 0x1C00 if lcdc & 0x08 else 0x1800
        winmap = 0x1C00 if lcdc & 0x40 else 0x1800
        for sy in range(144):
            for sx in range(160):
                usewin = (lcdc & 0x20) and sy >= wy and sx >= wx and self.io[0x4B] < 167 and wy < 144
                if usewin:
                    mx, my, mp = sx - wx, sy - wy, winmap
                else:
                    mx, my, mp = (sx + scx) & 255, (sy + scy) & 255, bgmap
                ti = (my >> 3) * 32 + (mx >> 3)
                tile = v0[mp + ti]
                attr = v1[mp + ti]
                x = mx & 7; y = my & 7
                if attr & 0x20: x = 7 - x
                if attr & 0x40: y = 7 - y
                if lcdc & 0x01 or cgb:
                    c = tilepix((attr >> 3) & 1, tile, x, y)
                else:
                    c = 0
                bgidx[sy][sx] = c
                bgprio[sy][sx] = bool(attr & 0x80)
                if cgb:
                    px[sx, sy] = self.color(self.bgpal, (attr & 7) * 4 + c)
                else:
                    px[sx, sy] = grays[(bgp >> (c * 2)) & 3]
        if lcdc & 0x02:
            h = 16 if lcdc & 0x04 else 8
            sprites = []
            for i in range(40):
                y = self.oam[i * 4] - 16; x = self.oam[i * 4 + 1] - 8
                t = self.oam[i * 4 + 2]; at = self.oam[i * 4 + 3]
                if y <= -h or y >= 144 or x <= -8 or x >= 160:
                    continue
                sprites.append((x, i, y, t, at))
            sprites.sort(key=lambda s: (-s[0], -s[1]))
            for x, i, y, t, at in sprites:
                if h == 16: t &= 0xFE
                for yy in range(h):
                    sy = y + yy
                    if not 0 <= sy < 144: continue
                    ry = (h - 1 - yy) if at & 0x40 else yy
                    tt = t + (ry >> 3)
                    vr = self.vram[(at >> 3) & 1] if cgb else self.vram[0]
                    lo = vr[tt * 16 + (ry & 7) * 2]; hi = vr[tt * 16 + (ry & 7) * 2 + 1]
                    for xx in range(8):
                        sx = x + xx
                        if not 0 <= sx < 160: continue
                        bx = xx if at & 0x20 else 7 - xx
                        c = ((hi >> bx) & 1) << 1 | ((lo >> bx) & 1)
                        if c == 0: continue
                        if (at & 0x80) and bgidx[sy][sx] != 0: continue
                        if cgb:
                            px[sx, sy] = self.color(self.obpal, (at & 7) * 4 + c)
                        else:
                            pal = self.io[0x49] if at & 0x10 else self.io[0x48]
                            px[sx, sy] = grays[(pal >> (c * 2)) & 3]
        return img

    def run_frames(self, n, joy_fn=None):
        target = self.frames + n
        while self.frames < target:
            if joy_fn:
                self.joy = joy_fn(self.frames)
            self.step()

    def shot(self, path, scale=3):
        im = self.render()
        im = im.resize((160 * scale, 144 * scale), Image.NEAREST)
        im.save(path)

R, L, U, D, A, B, SEL, START = 1, 2, 4, 8, 16, 32, 64, 128
