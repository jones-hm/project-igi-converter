import os
import io
import numpy as np
from .import IGI1_struct_iff
from .IGI1_struct_iff import *


def EXE(Chunks,File):
    

    ifftext = ""

    NameofAnim = (File[-10:-1])

#----------------------------------    

    boah_bytes = b''

    both_bytes = b''

    botd_bytes = b'' 

    borh_bytes = b'' 

    boeh_bytes = b'' 

    boed_bytes = b'' 

    bord_bytes = b''

    for i in range ( 0 , len(Chunks) ):

        Bytes = Chunks[i][4]

        if ( Chunks[i][0] == b'BOAH' ):
                                      boah_bytes += Bytes
        if ( Chunks[i][0] == b'BOTH' ):
                                      both_bytes += Bytes
        if ( Chunks[i][0] == b'BOTD' ):
                                      botd_bytes += Bytes
        if ( Chunks[i][0] == b'BORH' ):
                                      borh_bytes += Bytes
        if ( Chunks[i][0] == b'BOEH' ):
                                      boeh_bytes += Bytes
        if ( Chunks[i][0] == b'BOED' ):
                                      boed_bytes += Bytes
        if ( Chunks[i][0] == b'BORD' ):
                                      bord_bytes += Bytes
                                      

#------------------------------------

    Br =[]

    for i in range ( 0 , len(parse_borh(borh_bytes)) ):

      for j in range ( 0 , parse_borh(borh_bytes)['count'][i] ):

        k = "(0" + str(i) + ")"

        if ( i >= 10 ):

         k = "(" + str(i) + ")"

        Br.append(k)

    Data = str(parse_bord(bord_bytes)).replace("(","(00)(")

    for i in range ( 0 , len(Data) ):

      if ( Data[i:i+4] == "(00)" ):

        Data = Data[:i] + Br[0] + Data[i+4:]

        del Br[0]

    Datr = str(Data)

#------------------------------------

# Anim ID

    ifftext += "\n\\\\ Anim Category \n\n"   
    ifftext += "[(\"" + str(NameofAnim) + "\")]\n"   
    ifftext += "\n\n"     
    
    Dath = parse_boah(boah_bytes)
    Data = parse_boah(boah_bytes)

# Anim Header

    ifftext += "\n\\\\ Anim Header \n"   
    ifftext += "\n" + str(Data) + "\n"
    ifftext += "\n\n"     

    Dath = parse_boeh(boeh_bytes)
    Data = parse_boed(boed_bytes)
    Data = str(Data).replace(") ",")\n ")

# Bone Trigger events

    ifftext += "\n\\\\ Bone Trigger Events \n"
    ifftext += "\n" + str(Dath['count'])
    ifftext += "\n" + str(Data) + "\n"
    ifftext += "\n\n"

    Dath = parse_both(both_bytes)
    Data = parse_botd(botd_bytes)
    Data = str(Data).replace(") ",")\n ")

# Bone Translation Frames

    ifftext += "\n\\\\ Bone Translation Frames \n"
    ifftext += "\n" + str(Dath['count'])
    ifftext += "\n" + str(Data) + "\n"
    ifftext += "\n\n"

    Dath = parse_borh(borh_bytes)
    Data = parse_bord(bord_bytes)
    Data = str(Data).replace(") ",")\n ")

# Bone Rotation Frames

    ifftext += "\n\\\\ Bone Rotation Frames \n"
    ifftext += "\n" + str(Dath) + "\n"
    ifftext += "\n" + str(Datr) + "\n"
    ifftext += "\n"


#-------------------------------------------------
    

    N = (parse_boah(boah_bytes)[0][3])

    if  (N < 10):
                    N = "00"+str(N)
    elif (N < 100):
                    N = "0" +str(N)

    FILENAME = "anim_"+str(N)+".IFF"

    File += FILENAME
      
    os.makedirs(os.path.dirname(File), exist_ok=True)
  
    with open(File, 'w') as F:
        
     F.write(ifftext)
    

    return FILENAME
