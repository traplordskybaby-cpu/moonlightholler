import sys, struct
import os
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
from emu import GB
from PIL import Image
def load(path):
    d=open(path,'rb').read()
    g=GB(b'\x00'*0x8000)
    g.vram=[bytearray(d[0:0x2000]),bytearray(d[0x2000:0x4000])]
    g.oam=bytearray(d[0x4000:0x40A0])
    bg=d[0x40A0:0x40A0+64]; ob=d[0x40A0+64:0x40A0+128]
    g.bgpal=bytearray(bg); g.obpal=bytearray(ob)
    regs=d[0x40A0+128:0x40A0+136]
    g.io[0x40]=regs[0]; g.io[0x43]=regs[1]; g.io[0x42]=regs[2]; g.io[0x4B]=regs[3]; g.io[0x4A]=regs[4]
    g.io[0x47]=regs[5]; g.io[0x48]=regs[6]; g.io[0x49]=regs[7]
    g.cgb_pal_used=True
    return g
def shot(path,out,scale=3):
    g=load(path)
    im=g.render().resize((160*scale,144*scale),Image.NEAREST); im.save(out); return g
if __name__=='__main__':
    shot(sys.argv[1],sys.argv[2], int(sys.argv[3]) if len(sys.argv)>3 else 3)
