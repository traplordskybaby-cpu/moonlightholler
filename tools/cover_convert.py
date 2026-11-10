"""Convert the box art into a Game Boy Color picture: 20x16 tiles, 7 art palettes + 1 text palette."""
import numpy as np
from PIL import Image, ImageEnhance, ImageFilter
from sklearn.cluster import KMeans

from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
SRC = str(ROOT / 'art' / 'box.png')
NPAL = 7

def prepare(crop=(20, 1040), sharp=True):
    im = Image.open(SRC).convert('RGB')
    c = im.crop((0, crop[0], 1024, crop[1]))
    c = c.resize((320, 256), Image.LANCZOS)          # 2x oversample, then box down for crisp detail
    c = ImageEnhance.Color(c).enhance(1.25)
    c = ImageEnhance.Contrast(c).enhance(1.12)
    c = c.resize((160, 128), Image.BOX)
    if sharp:
        c = c.filter(ImageFilter.UnsharpMask(radius=1.0, percent=90, threshold=2))
    return c

def to555(a):
    return (np.clip(np.round(a / 255.0 * 31), 0, 31)).astype(int)

def from555(v):
    return np.round(v * 255.0 / 31).astype(float)

def lum(c):
    return c[..., 0] * 0.3 + c[..., 1] * 0.59 + c[..., 2] * 0.11

def best_assign(tile_px, pals):
    """tile_px: (N,64,3); pals: (P,4,3) -> (assignment, error)"""
    # distance from each pixel to each palette colour: (N,64,P,4)
    d = ((tile_px[:, :, None, None, :] - pals[None, None, :, :, :]) ** 2).sum(-1)
    e = d.min(-1).sum(1)            # (N,P)
    return e.argmin(1), e

def convert(img, npal=NPAL, iters=8, seed=3, dither=True):
    a = np.array(img).astype(float)
    th, tw = 16, 20
    tiles = np.zeros((th * tw, 64, 3))
    for ty in range(th):
        for tx in range(tw):
            tiles[ty * tw + tx] = a[ty*8:ty*8+8, tx*8:tx*8+8].reshape(64, 3)
    # per-tile 4-colour palettes (sorted by luminance) -> cluster into npal groups
    feats = []
    for t in tiles:
        km = KMeans(4, n_init=2, random_state=seed).fit(t)
        c = km.cluster_centers_
        c = c[np.argsort(lum(c))]
        feats.append(c.reshape(-1))
    feats = np.array(feats)
    km = KMeans(npal, n_init=8, random_state=seed).fit(feats)
    pals = km.cluster_centers_.reshape(npal, 4, 3)
    for it in range(iters):
        assign, _ = best_assign(tiles, pals)
        new = pals.copy()
        for p in range(npal):
            px = tiles[assign == p].reshape(-1, 3)
            if len(px) >= 4:
                k4 = KMeans(4, n_init=3, random_state=seed).fit(px)
                c = k4.cluster_centers_
                new[p] = c[np.argsort(lum(c))]
        pals = new
    # snap palettes to RGB555
    p555 = to555(pals)
    pals = from555(p555)
    assign, err = best_assign(tiles, pals)
    # ordered dither + nearest
    bayer = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0 - 0.5
    out_idx = np.zeros((th * tw, 8, 8), dtype=int)
    for n in range(th * tw):
        ty, tx = divmod(n, tw)
        pal = pals[assign[n]]
        for y in range(8):
            for x in range(8):
                px = a[ty*8+y, tx*8+x]
                if dither:
                    px = px + bayer[(ty*8+y) % 4, (tx*8+x) % 4] * 14
                d = ((pal - px) ** 2).sum(1)
                out_idx[n, y, x] = d.argmin()
    return p555, assign, out_idx

def render(p555, assign, idx):
    pals = from555(p555)
    im = Image.new('RGB', (160, 128))
    px = im.load()
    for n in range(320):
        ty, tx = divmod(n, 20)
        for y in range(8):
            for x in range(8):
                px[tx*8+x, ty*8+y] = tuple(int(v) for v in pals[assign[n]][idx[n, y, x]])
    return im

def write_cover(img, p555, assign, idx):
    import json, sys
    sys.path.insert(0, str(ROOT / 'tools'))
    from gbasset import tile_2bpp
    tiles = []
    for n in range(320):
        rows = [''.join(str(int(v)) for v in idx[n, y]) for y in range(8)]
        tiles.append(tile_2bpp(rows).hex())
    # the 'PRESS START' band borrows the palette with the darkest background and brightest highlight
    def score(i):
        lo, hi = from555(p555[i][0]), from555(p555[i][3])
        return lum(hi) - lum(lo) + 0.6 * ((hi[0] + hi[1]) / 2 - hi[2])  # bright on dark, and golden like the logo
    best = max(range(len(p555)), key=score)
    data = {'width_tiles': 20, 'height_tiles': 16, 'palettes': [[list(map(int, c)) for c in p] for p in p555],
            'tile_palette': [int(v) for v in assign], 'tiles': tiles, 'text_palette': int(best)}
    (ROOT / 'art' / 'cover.json').write_text(json.dumps(data))
    img.save(ROOT / 'art' / 'cover_prepared.png')
    return best


if __name__ == '__main__':
    base = prepare()
    a = np.array(base).astype(float) / 255
    a = np.clip((a ** 0.8 - 0.5) * 1.08 + 0.5, 0, 1)         # lift the shadows a little
    img = Image.fromarray((a * 255).astype('uint8'))
    img = ImageEnhance.Color(img).enhance(1.45)              # real GBC screens are washed out: push the colour
    p555, assign, idx = convert(img, npal=8, dither=True, seed=5)
    best = write_cover(img, p555, assign, idx)
    render(p555, assign, idx).resize((480, 384), Image.NEAREST).save(ROOT / 'art' / 'cover_preview.png')
    print('wrote art/cover.json (text palette %d) and art/cover_preview.png' % best)
