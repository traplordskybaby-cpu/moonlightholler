import sys
from PIL import Image
import areas, worldkit
which = sys.argv[1:] or ['farm','house']
for name in which:
    a = getattr(areas, 'build_'+name)()
    print(name, 'tiles', len(a.tiles), 'size', a.w, a.h)
    worldkit.render(a, scale=4, show_solid=True).save(f'/home/claude/game/build_prev_{name}.png')
