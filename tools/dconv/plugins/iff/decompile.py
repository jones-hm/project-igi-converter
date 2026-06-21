import os
from utils import fs
from format.iff import IFF
from . import iff
def func(args):
    count = 0

    for srcpath in fs.walkdir(args.src, '*.iff'):
        ifffile = IFF()
        iff.srcpth = srcpath
        ifffilename = (srcpath.replace(args.src, args.dst, 1).replace('.iff', '.IFF'))
        ifftext = iff.fromtree(ifffilename)

        dstfile = srcpath.replace(args.src, args.dst, 1).replace('.iff', '.IFF')
        os.makedirs(os.path.dirname(dstfile), exist_ok=True)
        
        with open(dstfile, 'w') as o:
            o.write(ifftext)

        count += 1


    print('\n Decompiled: {0}'.format(count))
