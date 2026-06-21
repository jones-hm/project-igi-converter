import os
from utils import fs
from .import IGI1_iffw
def func(args):
    count = 0

    for srcpath in fs.walkdir(args.src, '*.IFF'):

        if ( str(srcpath[len(args.src):]).find("\\anims_") != -1 ):

                continue

        if ( True ): # for Function (GET FILENAME)

            for i in range ( 0 , len(str(srcpath)) ):

              if ( str(srcpath)[i:].find("\\") == -1 ):

                FileSb = str(srcpath)[i:].replace(".IFF","")

                FileSb = "anims_" + str(FileSb)

                DIR = str(srcpath)[:i]
                    
                break

        if ( True ): # for Function (GET SUBDIRS NAMES)

            ListDr = []

            for i in os.listdir(DIR):

              F = os.path.join(DIR,i)  

              if os.path.isdir(F):

                ListDr.append(i)

            assert FileSb in ListDr , "Anims Folder Not Found, \"" + FileSb + "\\anim_%d.iff\""
        
        ifffilename = (srcpath.replace(args.src, args.dst, 1).replace('.IFF', '.iff'))
        IGI1_iffw.srcpth = srcpath
        BytesData = IGI1_iffw.fromtree(ifffilename,args.src)

        count += 1

    print('\n Created: {0}'.format(count))
