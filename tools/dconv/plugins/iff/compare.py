import os
from utils import fs
from format.iff import IFF
from . import comp_02
def func(args):
    
    count = 0

    text = "\n"

    text += " * ModelFileNames :- " + "\n\n"

    disc = "\n"

    Efile1 = 'IGI 2\\IffErrorList.qsc'

    Efile2 = 'IGI 2\\IffErrorDisc.qsc'
    
    count = 0

    for srcpath in fs.walkdir(args.src, '*.iff'):
        
        ifffile = IFF()
        
        srcpth1 = srcpath

        E = 0
        
        ifffile1 = os.path.basename(srcpth1)
        
        for srcpath in fs.walkdir(args.dst, ifffile1):

            E = 1

            srcpth2 = srcpath

            print("\n * Anim IFF ---- " + os.path.basename(ifffile1))

            disc += "\n * Anim IFF ---- " + os.path.basename(ifffile1)

        if E == 1 :
            
            text += str(ifffile1) + comp_02.fromtree(srcpth1,srcpth2)[0] + "\n"

            disc += comp_02.fromtree(srcpth1,srcpth2)[1]

            count += 1

            print("\n --------------------------------------")

            disc += "\n ------------------------------------"

        if E == 0 :

            print("\n - !Anim " + str(ifffile1) + " Not Found. ????????????????????????? ")

            disc += "\n - !Anim " + str(ifffile1) + " Not Found. ??????????????????????? "

        
    with open(Efile1, 'w') as o:
        
        o.write(text)

    with open(Efile2, 'w') as o:
        
        o.write(disc)            

    print('\n Compared: {0}'.format(count))
