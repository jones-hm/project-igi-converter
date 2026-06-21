import os 
import numpy as np
from .import reader_frm
from .import IGI1_struct_iff
from .reader_frm import *
from .IGI1_struct_iff import *

def fromtree(ifffilename,Pth):

    ifftext = ""

    Dname = os.path.splitext(os.path.basename(srcpth))[0]
    
    print ("\n Animation File IFF -- " + str(os.path.basename(ifffilename)))
    
    reader = reader_frm.open(srcpth,Pth,0)

# -----------------------------------------

    if reader.find(b'FORM'):
        
        anims = ""
        
# Initial text (Newobject)

        ifftext += "\n\\\\ Anim Name \n\n"
        ifftext += "[(\"" + str(os.path.basename(ifffilename)) + "\")]\n"
        ifftext += "\n\n"

        form_bytes = reader.read(b'FORM')  
        form = parse_form(form_bytes) 
        
# FORM

        ifftext += "\n\\\\ Bone Header \n\n"
        ifftext += str(form)
        ifftext += "\n\n"
        
        plst_bytes = reader.read(b'PLST')
        plst = parse_plst(plst_bytes)        
        
# PLST

        ifftext += "\n\\\\ Bone Links \n\n"
        ifftext += str(plst)
        ifftext += "\n\n"

        tlst_bytes = reader.read(b'TLST')
        tlst = parse_tlst(tlst_bytes)        
        
# TLST

        ifftext += "\n\\\\ Bone Hierarchy \n\n"
        ifftext += str(tlst)
        ifftext += "\n\n"

        form_bytes = reader.read(b'FORM',1)  
        form = parse_form(form_bytes) 

# FORM

        ifftext += "\n\n\\\\ Anims Header \n\n"
        ifftext += str(form)
        ifftext += "\n"

        for i in range ( 0 , len(Names) ):

         if ( i == 0 ):

          anims += "[(\""+"anims_"+str(Dname)+"\\"+str(Names[i])+"\")\n"

         elif i == len(Names)-1 :

          anims += " (\""+"anims_"+str(Dname)+"\\"+str(Names[i])+"\")]\n"

         else: # For Middle Order
            
          anims += " (\""+"anims_"+str(Dname)+"\\"+str(Names[i])+"\")\n"

# ANIM

        ifftext += "\n\n\\\\ Anims List \n\n"
        ifftext += str(anims)
        ifftext += "\n"
        
        
        
    return ifftext
