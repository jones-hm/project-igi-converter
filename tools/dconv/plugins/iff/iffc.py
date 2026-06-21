import os 
import numpy as np
from .import struct_iff
from .import reader_ilff
from .import iffc_DW
from .reader_ilff import *
from .iffc_DW import *
from .struct_iff import *

def fromtree(ifffilename):

    ifftext = ""
    
    reader = reader_ilff.open(srcpth)
    
    if reader.find(b'DHNA'):
        dhna_bytes = reader.read(b'DHNA')
        dhna = parse_dhna(dhna_bytes)
    
        if dhna['num_bones'] == 47:
             print ("\n * Anim Type is 1st Person anim - " + os.path.basename(ifffilename)) 
             ifftext = iffc_DW.fromfile(ifffilename,srcpth)

        elif dhna['num_bones'] == 31:
             print ("\n * Anim Type is 3rd Person anim - " + os.path.basename(ifffilename)) 
             ifftext = iffc_DW.fromfile(ifffilename,srcpth) 
        else:
             print ("\n * Anim Type is Undefined - " + os.path.basename(ifffilename)) 
             ifftext = input("Error!")      
        
    return ifftext

def fromtype(ifffilename):

    NamModel = str(os.path.splitext(os.path.basename(ifffilename))[0])
    
    ifftext = "CreateAnim(\"" + str(NamModel) + "\");\n"    
        
    return ifftext
