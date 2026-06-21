import os 
import numpy as np
from .import reader_frm
from .import reader_anims_frm
from .import IGI1_struct_iff
from .import IGI1_iffc_DW
from .reader_frm import *
from .reader_anims_frm import *
from .IGI1_struct_iff import *
from .IGI1_iffc_DW import *

def fromtree(ifffilename,Pth):

    qsctext = ""

    ArhFile = os.path.basename(ifffilename)

    ifffilename = ifffilename.replace(".BEF","")

    print("\n Animation File --- " + str(ArhFile))
    
    reader = reader_frm.open(srcpth,"",1)
    
    POS = reader.seek(b'TLST')[1]+reader.seek(b'TLST')[5]

    Size = reader.tellsize()

# --------------------------------------------

    form_bytes = reader.read(b'FORM')
    form = parse_form(form_bytes)
    
    plst_bytes = reader.read(b'PLST')
    plst = parse_plst(plst_bytes)
    
    tlst_bytes = reader.read(b'TLST')
    tlst = parse_tlst(tlst_bytes)

# -------<


    T = form['_count_00'][0]

    Bones = []

    for i in range ( 0 , len(plst) ):

     Name = "Bone_"+str(i)

     if i < 10 :
                Name = "Bone_0"+str(i)

     Bones.append([ Name , plst['Id_child'][i] , tlst['px'][i] , tlst['py'][i] , tlst['pz'][i] ])
     

# -------------------------------------------->

    Anims = []

    POS += 28

    while(True):

        AnimData = reader_anims_frm.GETDATA(POS,srcpth,Size,"",1)

        POS = AnimData[0]

        Anims.append(AnimData[1])

        if ( POS == Size ):

         break



# --------------------------------------------<

# --------------------------------------------


# DATA WRITING !


    while ( len(Anims) != 0 ):


# Decoding-->
  

        BytesData = Anims[0][0][4]
        boah = parse_boah(BytesData)
        
        BytesData = Anims[0][1][4]
        both = parse_both(BytesData)
        
        BytesData = Anims[0][2][4]
        botd = parse_botd(BytesData)

        BytesData1 = b''
        BytesData2 = b''

        for i in range ( 0 , len(tlst) ):
        
         BytesData1 += Anims[0][((i*2)+3)+0][4]
         BytesData2 += Anims[0][((i*2)+3)+1][4]

        borh = parse_borh(BytesData1)
        bord = parse_bord(BytesData2)
        
        BytesData = Anims[0][(i*2)+5][4]
        boeh = parse_boeh(BytesData)
        
        BytesData = Anims[0][(i*2)+6][4]
        boed = parse_boed(BytesData)

        del Anims[0] # To Execute Next One


# ------------------------------

        NameOFAnim = boah['_id'][0]

        if NameOFAnim < 10 :
                            NameOFAnim = "00"+str(NameOFAnim)
        elif NameOFAnim < 100 :
                            NameOFAnim = "0"+str(NameOFAnim)
        elif NameOFAnim < 1000 :
                            NameOFAnim = ""+str(NameOFAnim)

        NameOfAnim = "_anim_"+NameOFAnim

# ------------------------------


        Off = []
        
        intervals = []

        Tp = 0

        if ( boah['_01'][0] != 0 ):

             Tp = 1

        N = 0

        for i in range ( 0 , len(borh) ):

            Off.append(N)

            N += borh['count'][i]
            

        for i in range ( 0 , len(bord) ):

         if ( bord['time'][i] not in intervals ):

            intervals.append(bord['time'][i])


        for i in range ( 0 , len(intervals) ):

         for j in range ( i+1 , len(intervals) ):

           if ( intervals[i] > intervals[j] ):

               Temp_Variable = intervals[i]

               intervals[i] = intervals[j]

               intervals[j] = Temp_Variable


# ------------------------------

        if len(Bones) == 56 :
                                Bones,T = CalC_Bne0(Bones),CalC_Indices0()
        if len(Bones) == 33:
                                Bones,T = CalC_Bne1(Bones),CalC_Indices1()

# ------------------------------


        ifftext = ""

        Sc = 40.96

        Cat = os.path.splitext(ArhFile)[0]

        qsctext += "CreateAnim(\"" + "anims_"+str(Cat) + "\\" + Cat + str(NameOfAnim) + "\");\n"
        
        
# Anim Initialization Data for Conversion --------------->  


        AnimHder = "AnimInit(\"" + str(ArhFile.replace(".BEF","")) + str(NameOfAnim) + "\"" + "," + str(0) + "," + str(int(boah['length'][0])+1) + "," + str(Tp) + ");" + '\n'


# Bone Data for BEF Conversion ---------->


        Bone = ""

        for i in range ( 0 , len(Bones) ):

            Bone += "Bone(" + str(i) + ",\"" + str(Bones[i][0]) + "\"," + str(Bones[i][1]) + "," + str(Bones[i][2]/Sc) + "," + str(Bones[i][3]/Sc) + "," + str(Bones[i][4]/Sc) + ");\n"

        Bone += "BuildHierarchy();\n"


# TranslationalFrames Data for BEF Conversion ---------->
    

        Tframes = ""

        for i in range ( 0 , both['count'][0] ):

            Tframes += "TranslationKeyFrameData(" + str(0) + "," + str(0) + "," + str(int(botd['time'][i])) + "," + str((botd['px'][i])/Sc) + "," + str((botd['py'][i])/Sc) + "," + str((botd['pz'][i])/Sc) + ");" + '\n'


# Trigger Events Data for BEF Conversion ------------>


        TData = ""

        for i in range ( 0 , boeh['count'][0] ):

            TData += "TriggerData(" + str(i) + "," + str(boed['_id'][i]) + "," + str(int(boed['time'][i])) + "," + str(boed['BoneID'][i]) + "," + str((boed['px'][i])/Sc) + "," + str((boed['py'][i])/Sc) + "," + str((boed['pz'][i])/Sc) + ");" + '\n'


# AttachObject Frames -------------------------


        Rframes = ""

        for I in range ( 0 , len(intervals) ):

         for i in range ( 0 , len(borh) ):

          for j in range ( 0 , borh['count'][i] ):

           if intervals[I] == bord['time'][Off[i]+j] :

            if T[i] >= 0 :
                    Rframes += "RotationKeyFrameData(" + str(T[i]) + "," + str(0) + "," + str(int(bord['time'][Off[i]+j])) + "," + str(bord['_Ax0'][Off[i]+j]) + "," + str(bord['_Ay0'][Off[i]+j]) + "," + str(bord['_Az0'][Off[i]+j]) + "," + str(bord['_w00'][Off[i]+j]) + "," + str(bord['_Ax1'][Off[i]+j]) + "," + str(bord['_Ay1'][Off[i]+j]) + "," + str(bord['_Az1'][Off[i]+j]) + "," + str(bord['_w01'][Off[i]+j]) + "," + str(bord['_Ax2'][Off[i]+j]) + "," + str(bord['_Ay2'][Off[i]+j]) + "," + str(bord['_Az2'][Off[i]+j]) + "," + str(bord['_w02'][Off[i]+j]) + ");" + '\n'

            if T[i] == -10 and intervals[I] == 0.0 :
                    Rframes += "RotationKeyFrameData(10,0,0,7.9456095e-09,-5.2880295e-10,-1.9979498e-09,1.0,7.9456095e-09,-5.2880295e-10,-1.9979498e-09,1.0,7.9456095e-09,-5.2880295e-10,-1.9979498e-09,1.0);" + '\n'
                    Rframes += "RotationKeyFrameData(11,0,0,7.9456095e-09,-5.2880295e-10,-1.9979498e-09,1.0,7.9456095e-09,-5.2880295e-10,-1.9979498e-09,1.0,7.9456095e-09,-5.2880295e-10,-1.9979498e-09,1.0);" + '\n'
                    Rframes += "RotationKeyFrameData(12,0,0,7.9456095e-09,-5.2880295e-10,-1.9979498e-09,1.0,7.9456095e-09,-5.2880295e-10,-1.9979498e-09,1.0,7.9456095e-09,-5.2880295e-10,-1.9979498e-09,1.0);" + '\n'


##_______________________________________________________

# Anim Initialization

        ifftext += str(AnimHder)

# BreakScript

        ifftext += str("BreakScript();\n")

# Bones

        ifftext += str(Bone)

# BreakScript

        ifftext += str("BreakScript();\n")    

# Translational Frames

        ifftext += str(Tframes)

# Rotational Frames

        ifftext += str(Rframes)

# Trigger Data

        ifftext += str(TData)


# ------------------------------

        File = Pth+"\\"+"anims_"+str(Cat)+"\\"+Cat+str(NameOfAnim)+".BEF"

        print(" -> "+str(NameOfAnim[1:])+".BEF")

        WriteData = IGI1_iffc_DW.WRITE(File,ifftext) 
         
        
    return qsctext






# ------------------------------
# ----- FUNCTIONS DEFINITIONS --
# ------------------------------




def CalC_Indices1():

    return [ 0,1,4,7,15,-1,-1,20,16,21,23,25,27,29,17,22,24,26,28,30,-1,-1,-10,2,5,8,13,18,3,6,9,14,19 ]

def CalC_Indices0():

    return [ 0,-1,-1,-1,2,4,6,14,24,34,44,13,23,33,43,15,25,35,45,12,22,32,42,16,26,36,46,1,3,5,10,20,30,40,9,19,29,39,8,18,28,38,7,17,27,37,11,21,31,41,-1,-1,-1,-1,-1,-1 ]

def CalC_Bne1(Bone):
    
    Bnes = [ [0,-1],
             [1,0], [23,0],[28,0],
             [2,1], [24,2],[29,3],
             [3,4], [25,5],[30,6],
             [88,7],[88,7],[88,7],[26,8],[31,9],
             [4,10],[8,11],[14,12],[27,13],[32,14],
             [7,15],[9,16],[15,17],
             [10,21],[16,22],[11,23],[17,24],
             [12,25],[18,26],[13,27],[19,28] ]

    for i in range ( 0 , len(Bnes) ):
        I,V = Bnes[i][0], [ 0.0e+00, 0.0e+00, 0.0e+00 ]
        if I < len(Bone):
                   V = [ Bone[I][2], Bone[I][3], Bone[I][4] ]
        Bnes[i] =  [ Bone[i][0], Bnes[i][1], V[0],V[1],V[2] ]

    return Bnes


def CalC_Bne0(Bone):
    
    Bnes = [ [0,-1],
             [27,0],[4,0],[28,1],[5,2],[29,3],[6,4],
             [30,5], [34,5], [38,5], [42,5], [46,5],
             [7,6],  [11,6], [15,6], [19,6], [23,6],
             [31,7], [35,8], [39,9], [43,10],[47,11],
             [8,12], [12,13],[16,14],[20,15],[24,16],
             [32,17],[36,18],[40,19],[44,20],[48,21],
             [9, 22],[13,23],[17,24],[21,25],[25,26],
             [33,27],[37,28],[41,29],[45,30],[49,31],
             [10,32],[14,33],[18,34],[22,35],[26,36] ]

    for i in range ( 0 , len(Bnes) ):
        I,V = Bnes[i][0], [ 0.0e+00, 0.0e+00, 0.0e+00 ]
        if I < len(Bone):
                   V = [ Bone[I][2], Bone[I][3], Bone[I][4] ]
        Bnes[i] =  [ Bone[i][0], Bnes[i][1], V[0],V[1],V[2] ]

    return Bnes

