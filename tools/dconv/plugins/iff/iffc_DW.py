import os 
import numpy as np
from .import reader_ilff
from .import struct_iff
from .struct_iff import *
from .reader_ilff import *

def fromfile(ifffilename, srcpth):
    
    ifftext = ""
    
    reader = reader_ilff.open(srcpth)

    dhna_bytes = reader.read(b'DHNA')
    dhna = parse_dhna(dhna_bytes)
    
    reih_bytes = reader.read(b'REIH')
    reih1 = parse_reih(reih_bytes)[0]
    reih2 = parse_reih(reih_bytes)[1]
    
    tnve_bytes = reader.read(b'TNVE')
    
    if reader.find(b'ATTA'):
    
        atta_bytes = reader.read(b'ATTA')
        atta = parse_atta(atta_bytes)

    NameOfAnim = str(os.path.splitext(os.path.basename(ifffilename))[0])

    Sc = 40.96


# --------    



# Anim Initialization Data for Conversion ---------------    


    Anim = "AnimInit(\"" + str(NameOfAnim) + "\"" + "," + str(0) + "," + str(dhna['anim_duration'][0]+1) + "," + str(dhna['anim_type'][0]) + ");" + '\n'

    
# Anim Attachments for Conversion --------------------


    Attachments = ""

    if reader.find(b'ATTA'):

        for i in range ( 0 , len(atta) ):

            aname = str(atta['model_name'][i])

            Attachment1 = "AnimAttachObject(\"" + str(aname[2:-1]) + "\"," + str(atta['_n'][i]) + "," + str(atta['_00'][i]) + "," + str(atta['_01'][i]) + "," + str(atta['_02'][i]) + "," + str(atta['_03'][i]) + "," + str(atta['_04'][i]) + "," + str(atta['_05'][i]) + "," + str(atta['_06'][i]) + "," + str(atta['_07'][i]) + "," + str(atta['_08'][i]) + "," + str((atta['px'][i])/Sc) + "," + str((atta['py'][i])/Sc) + "," + str((atta['pz'][i])/Sc) + ");" + '\n'
            Attachment2 = "AnimAttachObjectBoneID(" + str(atta['_n'][i]) + "," + str(atta['_a'][i]) + ");" + '\n'
            
            Attachments += Attachment1 + Attachment2
            

# Bone Hierarchy for BEF Conversion ----------

    Bind = [ -1 ]

    for i in range ( 1 , len(reih1) ):

        for v in range ( 0 , reih1['num_child'][i-1] ):

            Bind.append(i-1)

# Bone Data for BEF Conversion ----------

    Bones = ""

    for i in range ( 0 , len(reih2) ):

        if i < 10 :

            o = "0"

        else:

            o = ""

        Bones += "Bone(" + str(i) + ",\"" + "Bone # " + str(o) + str(i) + "\"," + str(Bind[i]) + "," + str(reih2['px'][i]/Sc) + "," + str(reih2['py'][i]/Sc) + "," + str(reih2['pz'][i]/Sc) + ");\n"

    Bones += "BuildHierarchy();\n"

# Frames Data for BEF Conversion ---------->

    X = 0

    n = 0

    Tframes = ""

    Rframes = ""

    TData = ""

    Aframes = ""

    Levents = ""

    N = 0

    while ( X != len(tnve_bytes) ):

        P = np.frombuffer(tnve_bytes[X:X+1],  DTYPE_TNVE_0)
        

# Translational Frames --------------------

        if ( P['checkID'][0] == 3 ):

            P = np.frombuffer(tnve_bytes[X:X+24],   DTYPE_TNVE_1)
            X = X + 24

            Tframes += "TranslationKeyFrameData(" + str(P['_0'][0]) + "," + str(N) + "," + str(P['time'][0]) + "," + str((P['px'][0])/Sc) + "," + str((P['py'][0])/Sc) + "," + str((P['pz'][0])/Sc) + ");" + '\n'

            N += 1

# Rotational Frames ------------------------

        elif ( P['checkID'][0] == 4 ):
            
            P = np.frombuffer(tnve_bytes[X:X+72],   DTYPE_TNVE_2)
            X = X + 72

            Rframes += "RotationKeyFrameData(" + str(P['BoneID'][0]) + "," + str(0) + "," + str(P['time'][0]) + "," + str(P['_Ax0'][0]) + "," + str(P['_Ay0'][0]) + "," + str(P['_Az0'][0]) + "," + str(P['_w00'][0]) + "," + str(P['_Ax1'][0]) + "," + str(P['_Ay1'][0]) + "," + str(P['_Az1'][0]) + "," + str(P['_w01'][0]) + "," + str(P['_Ax2'][0]) + "," + str(P['_Ay2'][0]) + "," + str(P['_Az2'][0]) + "," + str(P['_w02'][0]) + ");" + '\n' 


# Trigger Data --------------------------------

        elif ( P['checkID'][0] == 6 ):
            
            P = np.frombuffer(tnve_bytes[X:X+32],   DTYPE_TNVE_3)
            X = X + 32

            TData += "TriggerData(" + str(n) + "," + str(P['_id'][0]) + "," + str(P['time'][0]) + "," + str(P['BoneID'][0]) + "," + str((P['px'][0])/Sc) + "," + str((P['py'][0])/Sc) + "," + str((P['pz'][0])/Sc) + ");" + '\n'

            n += 1

# AttachObject Frames -------------------------


        elif ( P['checkID'][0] == 7 ):

            P = np.frombuffer(tnve_bytes[X:X+44],   DTYPE_TNVE_4)
            X = X + 44

            Aframes += "AttachObjectKeyFrame(" + str(P['_an'][0]) + "," + str(0) + "," + str(P['time'][0]) + "," + str(P['_Ax'][0]) + "," + str(P['_Ay'][0]) + "," + str(P['_Az'][0]) + "," + str(P['_w0'][0]) + "," + str((P['px'][0])/Sc) + "," + str((P['py'][0])/Sc) + "," + str((P['pz'][0])/Sc) + ");" + '\n'


# Linked Events -----------------------------

        elif ( P['checkID'][0] == 1 ):

            P = np.frombuffer(tnve_bytes[X:X+68],   DTYPE_TNVE_5)
            X = X + 68

            Levents += "LinkEvent(" + str(P['_an'][0]) + "," + str(P['time'][0]) + "," + str(P['BoneID'][0]) + "," + str(P['_Ax0'][0]) + "," + str(P['_Ay0'][0]) + "," + str(P['_Az0'][0]) + "," + str(P['_Ax1'][0]) + "," + str(P['_Ay1'][0]) + "," + str(P['_Az1'][0]) + "," + str(P['_Ax2'][0]) + "," + str(P['_Ay2'][0]) + "," + str(P['_Az2'][0]) + "," + str((P['_px'][0])/Sc) + "," + str((P['_py'][0])/Sc) + "," + str((P['_pz'][0])/Sc) + ");" + '\n'

# ------------------------------------------

        elif ( P['checkID'][0] == -1 ):

            P = np.frombuffer(tnve_bytes[X:X+12],   DTYPE_TNVE_6)
            X = X + 12

##_______________________________________________________

# Anim Initialization

    ifftext += str(Anim)

# BreakScript

    ifftext += str("BreakScript();\n")

# Bones

    ifftext += str(Bones)

# Attachments

    ifftext += str(Attachments)

# BreakScript

    ifftext += str("BreakScript();\n")    

# Frames ->

# Translational Frames

    ifftext += str(Tframes)

# Rotational Frames

    ifftext += str(Rframes)

# Trigger Data

    ifftext += str(TData)

# AttachObject Frames

    ifftext += str(Aframes)

# Link Events

    ifftext += str(Levents)
    

    return ifftext
