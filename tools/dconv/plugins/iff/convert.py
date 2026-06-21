import os
from utils import fs
from format.iff import IFF
from . import iffc
def func(args):
    count = 0

    Mdfile = 'IGI 2\\Anims.qsc'

    qsctext = ""
    qsctext += "// Script for converting common models //////////////////////////////////////" + '\n'
    qsctext += "\n"
    qsctext += "// Script directories ///////////////////////////////////////////////////////" + '\n'    
    qsctext += "\n"
    qsctext += "SetAnimDirectory(\"anims\");\nSetModelDirectory(\"models\");\nSetTextureDirectory(\"textures\");\nSetPaletteDirectory(\"palettes\");\nSetTempDirectory(\"temp\");\n"
    qsctext += "\n"
    qsctext += "// Model settings /////////////////////////////////////////////////////////\n"
    qsctext += "\n"
    qsctext += "SetScale(40.96);\n"
    qsctext += "SetTargetPlatform(\"PC\");\n"
    qsctext += "\n"    
    qsctext += "// Texture settings /////////////////////////////////////////////////////////\n"
    qsctext += "\n"
    qsctext += "StartTexScript(\"commontex\");\n"
    qsctext += "\n"
    qsctext += "SetLightmapResolution(1);\n"
    qsctext += "\n"

    for srcpath in fs.walkdir(args.src, '*.iff'):
        ifffile = IFF()
        iffc.srcpth = srcpath
        ifffilename = (srcpath.replace(args.src, args.dst, 1).replace('.iff', '.BEF'))
        iffdata  = iffc.fromtree(ifffilename)
        qsctext += iffc.fromtype(ifffilename)

        dstfile = srcpath.replace(args.src, args.dst, 1).replace('.iff', '.BEF')
        os.makedirs(os.path.dirname(dstfile), exist_ok=True)
        
        with open(dstfile, 'w') as o:
            o.write(iffdata)

        count += 1

    qsctext += "\n"
    qsctext += "// End script ///////////////////////////////////////////////////////////////\n"
    qsctext += "\n"
    qsctext += "EndTexScript();\n"
    qsctext += "\n"
    qsctext += "BuildStatic(\"level\");"

    with open(Mdfile, 'w') as o:
        o.write(qsctext)

    print('\n Converted: {0}'.format(count))
