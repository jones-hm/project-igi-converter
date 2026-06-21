import os 
import numpy as np
from .import struct_iff
from .import reader_ilff
from .struct_iff import *
from .reader_ilff import *

def fromtree(ifffilename):

    ifftext = "" 
    
    reader = reader_ilff.open(srcpth)
    
    print ("\n Animation File IFF -- " + str(os.path.basename(ifffilename)))

    if reader.find(b'DHNA'):
            
# Initial text (Newobject)

        ifftext += "\n\\\\ Anim Name \n\n"
        ifftext += "[(\"" + str(os.path.basename(ifffilename)) + "\")]\n"
        ifftext += "\n\n"

        dhna_bytes = reader.read(b'DHNA')  
        dhna = parse_dhna(dhna_bytes) 
        
# DHNA

        ifftext += "\n\\\\ Anim Info \n\n"
        ifftext += str(dhna)
        ifftext += "\n\n"
        
        reih_bytes = reader.read(b'REIH')
        reih1 = parse_reih(reih_bytes)[0]   
        reih2 = parse_reih(reih_bytes)[1]
        tnve_bytes = reader.read(b'TNVE')
        
# REIH

        ifftext += "\n\n\\\\ Bone Links \n\n"
        ifftext += str(reih1)
        ifftext += "\n\n\\\\ Bone Hierarchy \n\n"                      
        ifftext += str(reih2)
        ifftext += "\n\n"

        botf = parse_tnve(tnve_bytes)[0]     
        botf = str(botf).replace(") ",")\n ")
        
# BOTF

        ifftext += "\n\n\n\\\\ Translational KeyFrames \n\n"
        ifftext += str(botf)
  
        borf = parse_tnve(tnve_bytes)[1]
        borf = str(borf).replace(") ",")\n ")
        
# BORF

        ifftext += "\n\n\n\\\\ Rotational KeyFrames \n\n"
        ifftext += str(borf)
        
        botd = parse_tnve(tnve_bytes)[2]
        botd = str(botd).replace(") ",")\n ")
        
# BOTD

        ifftext += "\n\n\n\\\\ TriggerData \n\n"
        ifftext += str(botd)
        
        boaf = parse_tnve(tnve_bytes)[3]
        boaf = str(boaf).replace(") ",")\n ")
        
# BOAF

        ifftext += "\n\n\n\\\\ AttachObject KeyFrames \n\n"
        ifftext += str(boaf)
        
        bole = parse_tnve(tnve_bytes)[4]
        bole = str(bole).replace(") ",")\n ")
        
# BOLE

        ifftext += "\n\n\n\\\\ Link Events \n\n"
        ifftext += str(bole)
        
        banm = parse_tnve(tnve_bytes)[5] 
        banm = str(banm).replace(") ",")\n ") 
        
# BANM

        ifftext += "\n\n\n\\\\ Anim Data Info \n\n"
        ifftext += str(banm)
        
        if reader.find(b'ATTA'):
            
            atta_bytes = reader.read(b'ATTA')
            atta = parse_atta(atta_bytes)
            atta = str(atta).replace(") ",")\n ")

# ATTA

            ifftext += "\n\n\n\\\\ Attachments \n\n"
            ifftext += str(atta)
            ifftext += "\n"
            
        
    return ifftext
