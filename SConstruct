#! /bin/env python

from yoctools import Make

# defconfig = Make(elf='yoc.elf', objcopy='generated/data/prim', objdump='yoc.asm')
defconfig = Make(elf='yoc.elf', objcopy='generated/data/prim')

Export('defconfig')

defconfig.build_components()
