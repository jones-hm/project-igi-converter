import os 
import numpy as np
from .import reader_IFF
from .import reader_IFFS
from .import IGI1_iffw_DW
from .reader_IFF import *
from .reader_IFFS import *
from .IGI1_iffw_DW import *

def CalSize(Size):
        
    B = struct.pack('=1I',Size)

    Sz = struct.pack('=4B',B[3],B[2],B[1],B[0])

    return struct.unpack('=1I',Sz)[0]


def fromtree(ifffilename,Pth):

    Data = ""

    ArhFile = os.path.basename(ifffilename)

    print("\n Animation File --- " + str(ArhFile))

# ----------------------------------------->


    Data = reader_IFF.read(srcpth,Pth)

    Typ_Anims = Data[0][0]

    Num_Bones = Data[0][1]

    BonesC = Data[1][0]

    BonesH = Data[2][0]

    Set = []

    for i in range ( 0 , len(BonesH) // 3 ):

     Set.append([ BonesH[3*i+0], BonesH[3*i+1], BonesH[3*i+2] ])

    BonesH = Set

    Num_Anims = Data[3][0]

    Lid_Anims = Data[3][1]

    Lst_Anims = Data[4][0]
     

# -------------------------------------------->


    Anims = []

    for i in range ( 0 , len(Lst_Anims) ):

        Data = reader_IFFS.read(Lst_Anims[i],Pth,Num_Bones)

        HdrAn = Data[0][1]

        BTrgH = Data[1][0]

        BTrgD = Data[1][1]

        Set = []

        for i in range ( 0 , len(BTrgD) // 6 ):

          Set.append([ BTrgD[6*i+0], BTrgD[6*i+1], BTrgD[6*i+2], BTrgD[6*i+3], BTrgD[6*i+4], BTrgD[6*i+5] ])

        BTrgD = Set

        BTrnH = Data[2][0]

        BTrnD = Data[2][1]

        Set = []

        for i in range ( 0 , len(BTrnD) // 10 ):

         Set.append([ BTrnD[10*i+0], BTrnD[10*i+1], BTrnD[10*i+2], BTrnD[10*i+3], BTrnD[10*i+4], BTrnD[10*i+5], BTrnD[10*i+6], BTrnD[10*i+7], BTrnD[10*i+8], BTrnD[10*i+9] ])

        BTrnD = Set

        BRotH = Data[3][0]

        BRotD = Data[3][1]

        Set = []

        for i in range ( 0 , len(BRotD) // 14 ):

         Set.append([ BRotD[14*i+0], BRotD[14*i+1], BRotD[14*i+2], BRotD[14*i+3], BRotD[14*i+4], BRotD[14*i+5], BRotD[14*i+6], BRotD[14*i+7], BRotD[14*i+8], BRotD[14*i+9], BRotD[14*i+10], BRotD[14*i+11], BRotD[14*i+12], BRotD[14*i+13] ])

        BRotD = Set

        Anims.append([ HdrAn, BTrgH, BTrgD, BTrnH, BTrnD, BRotH, BRotD ])
        

# ----------------------------< Checks
 
 
    if ( len(Anims) != Num_Anims  ):
                               assert 0 != 0 , "Wrong Data Found in File"
 
    if ( len(BonesC) != Num_Bones  ):
                               assert 0 != 0 , "Wrong Data Found in File"
 
    if ( len(BonesH) != Num_Bones  ):
                               assert 0 != 0 , "Wrong Data Found in File"
 
    if ( len(Lst_Anims) != Num_Anims  ):
                               assert 0 != 0 , "Wrong Data Found in File"
                               


# -----------------------------< Other Stuff


    Off_Trn = []

    for i in range ( 0 , len(Anims) ):

      Off = []  

      Off.append(0)  

      for j in range ( 0 , len(Anims[i][3])-1 ):

        Off.append(Anims[i][3][j]+Off[len(Off)-1])

      Off_Trn.append(Off)  


    Off_Rot = []

    for i in range ( 0 , len(Anims) ):

      Off = []  

      Off.append(0)  

      for j in range ( 0 , len(Anims[i][5])-1 ):

        Off.append(Anims[i][5][j]+Off[len(Off)-1])

      Off_Rot.append(Off)

    Off_Trg = []

    for i in range ( 0 , len(Anims) ):

      Off = []  

      Off.append(0)

      for j in range ( 0 , len(Anims[i][1])-1 ):

        Off.append(Anims[i][1][j]+Off[len(Off)-1])

      Off_Trg.append(Off)  
        


# ---------------------------Compiling Bones (OBJS)


    Data_BNDT = b''

    Bytes = b''

    Sz = 0

    for i in  range ( 0 , 1 ):

        Bytes += struct.pack('=2I',Typ_Anims,Num_Bones)

        Sz += 8

    Data_BNDT += struct.pack('=1I',CalSize(Sz)) + Bytes


    Data_PLST = b'PLST'

    Bytes = b''

    Sz = 0

    for i in  range ( 0 , len(BonesC) ):

        Bytes += struct.pack('=1i',BonesC[i])

        Sz += 4

    Data_PLST += struct.pack('=1I',CalSize(Sz)) + Bytes


    Data_TLST = b'TLST'

    Bytes = b''

    Sz = 0

    for i in  range ( 0 , len(BonesH) ):

        Bytes += struct.pack('=3f', BonesH[i][0],BonesH[i][1],BonesH[i][2])

        Sz += 12

    Data_TLST += struct.pack('=1I',CalSize(Sz)) + Bytes



# ---------------------------Compiling Anims (FORMS)


    Data_AMDT = b''

    Bytes = b''

    Sz = 0

    for i in range ( 0 , 1 ):

        Bytes += struct.pack('=2I',Num_Anims,Lid_Anims)

        Sz += 8

    Data_AMDT += struct.pack('=1I',CalSize(Sz)) + Bytes



# ---------------------------Compiling Anims (FORMS DATA FILE BY FILE)

    

    Data_FORMS = []

    Bytes = b''

    Size = 0

    for i in range ( 0 , len(Anims) ):

        Bytes = b''


# ANIM INITIALS

        
        bytesh = b''

        Szh = 0

        for j in range ( 0 , 1 ):

            bytesh += struct.pack('=1f2H1I',Anims[i][0][0],Anims[i][0][1],Anims[i][0][2],Anims[i][0][3])

            Szh += 12
        
        Bytes += b'BOANBOAH' + struct.pack('=1I',CalSize(Szh)) + bytesh

        
# TRANSLATION FRAME


        bytesh = b''

        for j in range ( 0 , len(Anims[i][3]) ):

          Bytes += b'BOTH' + struct.pack('=1I',CalSize(4)) + struct.pack('=1I',(Anims[i][3][j]))

          bytesh = b''

          Szh = 0

          for k in range ( Off_Trn[i][j] , Off_Trn[i][j] + Anims[i][3][j] ):

            bytesh += struct.pack('=10f',Anims[i][4][k][0],Anims[i][4][k][1],Anims[i][4][k][2],Anims[i][4][k][3],Anims[i][4][k][4],Anims[i][4][k][5],Anims[i][4][k][6],Anims[i][4][k][7],Anims[i][4][k][8],Anims[i][4][k][9])

            Szh += 40
        
          Bytes += b'BOTD' + struct.pack('=1I',CalSize(Szh)) + bytesh

        
# ROTATION FRAME


        bytesh = b''

        for j in range ( 0 , len(Anims[i][5]) ):

          Bytes += b'BORH' + struct.pack('=1I',CalSize(4)) + struct.pack('=1I',(Anims[i][5][j]))

          bytesh = b''

          Szh = 0

          for k in range ( Off_Rot[i][j] , Off_Rot[i][j] + Anims[i][5][j] ):

            bytesh += struct.pack('=13f',Anims[i][6][k][1],Anims[i][6][k][2],Anims[i][6][k][3],Anims[i][6][k][4],Anims[i][6][k][5],Anims[i][6][k][6],Anims[i][6][k][7],Anims[i][6][k][8],Anims[i][6][k][9],Anims[i][6][k][10],Anims[i][6][k][11],Anims[i][6][k][12],Anims[i][6][k][13])

            Szh += 52
        
          Bytes += b'BORD' + struct.pack('=1I',CalSize(Szh)) + bytesh

        
# TRIGGER EVENTS


        bytesh = b''

        for j in range ( 0 , len(Anims[i][1]) ):

          Bytes += b'BOEH' + struct.pack('=1I',CalSize(4)) + struct.pack('=1I',(Anims[i][1][j]))

          bytesh = b''

          Szh = 0

          for k in range ( Off_Trg[i][j] , Off_Trg[i][j] + Anims[i][1][j] ):

            bytesh += struct.pack('=2i4f',Anims[i][2][k][0],Anims[i][2][k][1],Anims[i][2][k][2],Anims[i][2][k][3],Anims[i][2][k][4],Anims[i][2][k][5])

            Szh += 24
        
          Bytes += b'BOED' + struct.pack('=1I',CalSize(Szh)) + bytesh
          
        Bytes = b'FORM' + struct.pack('=1I',CalSize(len(Bytes))) + Bytes

        Data_FORMS.append(Bytes)

        if ( i == len(Anims)-1 ):

         break

        Size += len(Bytes)

    Size_AMHD = struct.pack('=1I',CalSize(Size+32))



#------------------------- OTHER SIZES CALCULATION

    Size = 4


    Size += 8

    for i in  range ( 0 , 1 ):

        Size += 8

    Size += 8

    for i in  range ( 0 , len(BonesC) ):

        Size += 4

    Size += 8

    for i in  range ( 0 , len(BonesH) ):

        Size += 12

    Size_BNHD = struct.pack('=1I',CalSize(Size))
    

    Size += 8

    for i in  range ( 0 , 1 ):

        Size += 4

    Size += 8

    for i in  range ( 0 , 1 ):

        Size += 4
    

    Size_FLHD = struct.pack('=1I',CalSize(Size))


    
# ------------------------------

    if (True):
        
# ------------------------------

# DATA WRITING !



        FLHD = b''

        BNHD = b''

        BNDT = b''

        AMHD = b''

        AMDT = b''
        
        
        
# FILE HEADER Conv. --------------->  


        FLHD += b'FORM'

        FLHD += Size_FLHD

        FLHD += b'BOBJ'
        

# BONE HEADER Conv. ------------>



        BNHD += b'FORM'

        BNHD += Size_BNHD

        BNHD += b'BOBH'


# BONE DATA CONVERSION ---------->


        BNDT += b'BOSH'

        BNDT += Data_BNDT

        BNDT += Data_PLST

        BNDT += Data_TLST


# ANIMS HEADER Conv. ------------>


        AMHD += b'FORM'

        AMHD += Size_AMHD

        AMHD += b'BOAL'

        
# ANIMS DATA CONVERSION ------------>


        AMDT += b'BALH'

        AMDT += Data_AMDT

        for i in range ( 0 , len(Anims) ):

         AMDT += Data_FORMS[i]


         

##_______________________________________________________
            

        Bytes = b''    

# FILE HEADER

        Bytes += FLHD

# BONE HEADER

        Bytes += BNHD

# BONE DATA

        Bytes += BNDT

# ANIMS HEADER

        Bytes += AMHD   

# ANIMS DATA

        Bytes += AMDT 




# ------------------------------

        File = ifffilename

        WriteData = IGI1_iffw_DW.WRITE(File,Bytes) 
         
        
    return None
