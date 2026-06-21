import os 
import numpy as np
from .import struct_iff
from .import reader_ilff
from .struct_iff import *
from .reader_ilff import *

def fromtree(srcpth1,srcpth2):

    Et = ""

    disc = ""

    reader = reader_ilff.open(srcpth1)

    if reader.find(b'DHNA'):

        dhna1_bytes = reader.read(b'DHNA')
        reih1_bytes = reader.read(b'REIH')
        tnve1_bytes = reader.read(b'TNVE')

        dhna1 = parse_dhna(dhna1_bytes)
        reih11 = parse_reih(reih1_bytes)[0]
        reih21 = parse_reih(reih1_bytes)[1] 
        botf1 = parse_tnve(tnve1_bytes)[0]
        borf1 = parse_tnve(tnve1_bytes)[1]
        botd1 = parse_tnve(tnve1_bytes)[2]
        boaf1 = parse_tnve(tnve1_bytes)[3]
        bole1 = parse_tnve(tnve1_bytes)[4]
        banm1 = parse_tnve(tnve1_bytes)[5]

        if reader.find(b'ATTA'):
            
            atta1_bytes = reader.read(b'ATTA')
            atta1 = parse_atta(atta1_bytes)

        else:

            atta1 = []

    reader = reader_ilff.open(srcpth2)

    if reader.find(b'DHNA'):

        dhna2_bytes = reader.read(b'DHNA')
        reih2_bytes = reader.read(b'REIH')
        tnve2_bytes = reader.read(b'TNVE')

        dhna2 = parse_dhna(dhna2_bytes)
        reih12 = parse_reih(reih2_bytes)[0]
        reih22 = parse_reih(reih2_bytes)[1] 
        botf2 = parse_tnve(tnve2_bytes)[0]
        borf2 = parse_tnve(tnve2_bytes)[1]
        botd2 = parse_tnve(tnve2_bytes)[2]
        boaf2 = parse_tnve(tnve2_bytes)[3]
        bole2 = parse_tnve(tnve2_bytes)[4]
        banm2 = parse_tnve(tnve2_bytes)[5]

        if reader.find(b'ATTA'):
            
            atta2_bytes = reader.read(b'ATTA')
            atta2 = parse_atta(atta2_bytes)

        else:

            atta2 = []


#------------------------------------------------
            
    n = 1

    if ( len(dhna1) == len(dhna2) ):

        for i in range ( 0 , len(dhna1) ):
            
            if dhna1['anim_type'][i] != dhna2['anim_type'][i] :

                print("\n -> dhna['anim_type'][" + str(i) + "] != dhna['anim_type'][" + str(i) + "]  " + " , " + str(dhna1['anim_type'][i]) + " != " + str(dhna2['anim_type'][i]))

                disc += "\n -> dhna['anim_type'][" + str(i) + "] != dhna['anim_type'][" + str(i) + "]  " + " , " + str(dhna1['anim_type'][i]) + " != " + str(dhna2['anim_type'][i])

                E = 0

                n = 0

            if abs(dhna1['anim_duration'][i] - dhna2['anim_duration'][i]) >= 1 :

                print("\n -> dhna['anim_duration'][" + str(i) + "] != dhna['anim_duration'][" + str(i) + "]  " + " , " + str(dhna1['anim_duration'][i]) + " != " + str(dhna2['anim_duration'][i]))

                disc += "\n -> dhna['anim_duration'][" + str(i) + "] != dhna['anim_duration'][" + str(i) + "]  " + " , " + str(dhna1['anim_duration'][i]) + " != " + str(dhna2['anim_duration'][i])

                E = 0

                n = 0

            if abs(dhna1['num_bones'][i] - dhna2['num_bones'][i]) >= 1 :

                print("\n -> dhna['num_bones'][" + str(i) + "] != dhna['num_bones'][" + str(i) + "]  " + " , " + str(dhna1['num_bones'][i]) + " != " + str(dhna2['num_bones'][i]))

                disc += "\n -> dhna['num_bones'][" + str(i) + "] != dhna['num_bones'][" + str(i) + "]  " + " , " + str(dhna1['num_bones'][i]) + " != " + str(dhna2['num_bones'][i])

                E = 0

                n = 0

            if abs(dhna1['num_frames'][i] - dhna2['num_frames'][i]) >= 1 :

                print("\n -> dhna['num_frames'][" + str(i) + "] != dhna['num_frames'][" + str(i) + "]  " + " , " + str(dhna1['num_frames'][i]) + " != " + str(dhna2['num_frames'][i]))

                disc += "\n -> dhna['num_frames'][" + str(i) + "] != dhna['num_frames'][" + str(i) + "]  " + " , " + str(dhna1['num_frames'][i]) + " != " + str(dhna2['num_frames'][i])

                E = 0

                n = 0

            if dhna1['num_attach'][i] != dhna2['num_attach'][i] :

                print("\n -> dhna['num_attach'][" + str(i) + "] != dhna['num_attach'][" + str(i) + "]  " + " , " + str(dhna1['num_attach'][i]) + " != " + str(dhna2['num_attach'][i]))

                disc += "\n -> dhna['num_attach'][" + str(i) + "] != dhna['num_attach'][" + str(i) + "]  " + " , " + str(dhna1['num_attach'][i]) + " != " + str(dhna2['num_attach'][i])

                E = 0

                n = 0

            if dhna1['_00'][i] != dhna2['_00'][i] :

                print("\n -> dhna['_00'][" + str(i) + "] != dhna['_00'][" + str(i) + "]  " + " , " + str(dhna1['_00'][i]) + " != " + str(dhna2['_00'][i]))

                disc += "\n -> dhna['_00'][" + str(i) + "] != dhna['_00'][" + str(i) + "]  " + " , " + str(dhna1['_00'][i]) + " != " + str(dhna2['_00'][i])

                E = 0

                n = 0

    if n == 0 :

        Et += " 'DHNA' "

#------------------------------------------------

    n = 1

    if ( len(reih11) != len(reih12) ):

        print("\n -> len(REIH) != len(REIH) ")

        disc += "\n -> len(REIH) != len(REIH) "

        E = 0

        n = 0

    else:

        for i in range ( 0 , len(reih11) ):
            
            if reih11['num_child'][i] != reih12['num_child'][i] :

                print("\n -> reih['num_child'][" + str(i) + "] != reih['num_child'][" + str(i) + "]  " + " , " + str(reih11['num_child'][i]) + " != " + str(reih12['num_child'][i]))

                disc += "\n -> reih['num_child'][" + str(i) + "] != reih['num_child'][" + str(i) + "]  " + " , " + str(reih11['num_child'][i]) + " != " + str(reih12['num_child'][i])

                E = 0

                n = 0

    if ( len(reih21) != len(reih22) ):

        print("\n -> len(REIH) != len(REIH) ")

        disc += "\n -> len(REIH) != len(REIH) "

        E = 0

        n = 0

    else:

        for i in range ( 0 , len(reih21) ):
            

            if abs(reih21['px'][i] - reih22['px'][i]) >= 1 :

                print("\n -> reih['px'][" + str(i) + "] != reih['px'][" + str(i) + "]  " + " , " + str(reih21['px'][i]) + " != " + str(reih22['px'][i]))

                disc += "\n -> reih['px'][" + str(i) + "] != reih['px'][" + str(i) + "]  " + " , " + str(reih21['px'][i]) + " != " + str(reih22['px'][i])

                E = 0

                n = 0

            if abs(reih21['py'][i] - reih22['py'][i]) >= 1 :

                print("\n -> reih['py'][" + str(i) + "] != reih['py'][" + str(i) + "]  " + " , " + str(reih21['py'][i]) + " != " + str(reih22['py'][i]))

                disc += "\n -> reih['py'][" + str(i) + "] != reih['py'][" + str(i) + "]  " + " , " + str(reih21['py'][i]) + " != " + str(reih22['py'][i])

                E = 0

                n = 0

            if abs(reih21['pz'][i] - reih22['pz'][i]) >= 1 :

                print("\n -> reih['pz'][" + str(i) + "] != reih['pz'][" + str(i) + "]  " + " , " + str(reih21['pz'][i]) + " != " + str(reih22['pz'][i]))

                disc += "\n -> reih['pz'][" + str(i) + "] != reih['pz'][" + str(i) + "]  " + " , " + str(reih21['pz'][i]) + " != " + str(reih22['pz'][i])

                E = 0

                n = 0
                

    if n == 0 :

        Et += " 'REIH' "

#------------------------------------------------

    n = 1

    if ( len(tnve1_bytes) != len(tnve2_bytes) ):

        print("\n -> len(TNVE) != len(TNVE) ")

        disc += "\n -> len(TNVE) != len(TVNE) "

        E = 0

        n = 0

    else:

          if ( dhna1_bytes != dhna2_bytes ):  

            print("\n -> (dhna1) != (dhna2) ")

            disc += "\n -> (dhna1) != (dhna2) "

            E = 0

            n = 0


    if n == 0 :

        Et += " 'TNVE' "

# ------------------------>

    n = 1

    if ( len(botf1) != len(botf2) ):

        print("\n -> len(BOTF) != len(BOTF) ")

        disc += "\n -> len(BOTF) != len(BOTF) "

        E = 0

        n = 0

    else:

        for i in range ( 0 , len(botf1) ):
            

            if (abs(botf1['px'][i] - botf2['px'][i])) >= 0.1 :

                print("\n -> botf1['px'][" + str(i) + "] != botf2['px'][" + str(i) + "]  " + " , " + str(botf1['px'][i]) + " != " + str(botf2['px'][i]))

                disc += "\n -> botf1['px'][" + str(i) + "] != botf2['px'][" + str(i) + "]  " + " , " + str(botf1['px'][i]) + " != " + str(botf2['px'][i])

                E = 0

                n = 0
            

            if abs(botf1['py'][i] - botf2['py'][i]) >= 0.1 :

                print("\n -> botf1['py'][" + str(i) + "] != botf2['py'][" + str(i) + "]  " + " , " + str(botf1['py'][i]) + " != " + str(botf2['py'][i]))

                disc += "\n -> botf1['py'][" + str(i) + "] != botf2['py'][" + str(i) + "]  " + " , " + str(botf1['py'][i]) + " != " + str(botf2['py'][i])

                E = 0

                n = 0
            

            if abs(botf1['pz'][i] - botf2['pz'][i]) >= 0.1 :

                print("\n -> botf1['pz'][" + str(i) + "] != botf2['pz'][" + str(i) + "]  " + " , " + str(botf1['pz'][i]) + " != " + str(botf2['pz'][i]))

                disc += "\n -> botf1['pz'][" + str(i) + "] != botf2['pz'][" + str(i) + "]  " + " , " + str(botf1['pz'][i]) + " != " + str(botf2['pz'][i])

                E = 0

                n = 0
            

            if (botf1['ID'][i] != botf2['ID'][i]) :

                print("\n -> botf1['ID'][" + str(i) + "] != botf2['ID'][" + str(i) + "]  " + " , " + str(botf1['ID'][i]) + " != " + str(botf2['ID'][i]))

                disc += "\n -> botf1['ID'][" + str(i) + "] != botf2['ID'][" + str(i) + "]  " + " , " + str(botf1['ID'][i]) + " != " + str(botf2['ID'][i])

                E = 0

                n = 0
            

            if (botf1['_0'][i] != botf2['_0'][i]) :

                print("\n -> botf1['_0'][" + str(i) + "] != botf2['_0'][" + str(i) + "]  " + " , " + str(botf1['_0'][i]) + " != " + str(botf2['_0'][i]))

                disc += "\n -> botf1['_0'][" + str(i) + "] != botf2['_0'][" + str(i) + "]  " + " , " + str(botf1['_0'][i]) + " != " + str(botf2['_0'][i])

                E = 0

                n = 0
            

            if (botf1['offset'][i] != botf2['offset'][i]) :

                print("\n -> botf1['offset'][" + str(i) + "] != botf2['offset'][" + str(i) + "]  " + " , " + str(botf1['offset'][i]) + " != " + str(botf2['offset'][i]))

                disc += "\n -> botf1['offset'][" + str(i) + "] != botf2['offset'][" + str(i) + "]  " + " , " + str(botf1['offset'][i]) + " != " + str(botf2['offset'][i])

                E = 0

                n = 0
            

            if (botf1['time'][i] != botf2['time'][i]) :

                print("\n -> botf1['time'][" + str(i) + "] != botf2['time'][" + str(i) + "]  " + " , " + str(botf1['time'][i]) + " != " + str(botf2['time'][i]))

                disc += "\n -> botf1['time'][" + str(i) + "] != botf2['time'][" + str(i) + "]  " + " , " + str(botf1['time'][i]) + " != " + str(botf2['time'][i])

                E = 0

                n = 0
            

            if (botf1['timer'][i] != botf2['timer'][i]) :

                print("\n -> botf1['timer'][" + str(i) + "] != botf2['timer'][" + str(i) + "]  " + " , " + str(botf1['timer'][i]) + " != " + str(botf2['timer'][i]))

                disc += "\n -> botf1['timer'][" + str(i) + "] != botf2['timer'][" + str(i) + "]  " + " , " + str(botf1['timer'][i]) + " != " + str(botf2['timer'][i])

                E = 0

                n = 0


    if n == 0 :

        Et += " 'BOTF' "


# ------------------------>

    n = 1

    if ( len(borf1) != len(borf2) ):

        print("\n -> len(BORF) != len(BORF) ")

        disc += "\n -> len(BORF) != len(BORF) "

        E = 0

        n = 0

    else:

        for i in range ( 0 , len(borf1) ):
            

            if abs(borf1['_Ax0'][i] - borf2['_Ax0'][i]) >= 0.0000001 :

                print("\n -> borf1['_Ax0'][" + str(i) + "] != borf2['_Ax0'][" + str(i) + "]  " + " , " + str(borf1['_Ax0'][i]) + " != " + str(borf2['_Ax0'][i]))

                disc += "\n -> borf1['_Ax0'][" + str(i) + "] != borf2['_Ax0'][" + str(i) + "]  " + " , " + str(borf1['_Ax0'][i]) + " != " + str(borf2['_Ax0'][i])

                E = 0

                n = 0
            

            if abs(borf1['_Ay0'][i] - borf2['_Ay0'][i]) >= 0.0000001 :

                print("\n -> borf1['_Ay0'][" + str(i) + "] != borf2['_Ay0'][" + str(i) + "]  " + " , " + str(borf1['_Ay0'][i]) + " != " + str(borf2['_Ay0'][i]))

                disc += "\n -> borf1['_Ay0'][" + str(i) + "] != borf2['_Ay0'][" + str(i) + "]  " + " , " + str(borf1['_Ay0'][i]) + " != " + str(borf2['_Ay0'][i])

                E = 0

                n = 0
            

            if abs(borf1['_Az0'][i] - borf2['_Az0'][i]) >= 0.0000001 :

                print("\n -> borf1['_Az0'][" + str(i) + "] != borf2['_Az0'][" + str(i) + "]  " + " , " + str(borf1['_Az0'][i]) + " != " + str(borf2['_Az0'][i]))

                disc += "\n -> borf1['_Az0'][" + str(i) + "] != borf2['_Az0'][" + str(i) + "]  " + " , " + str(borf1['_Az0'][i]) + " != " + str(borf2['_Az0'][i])

                E = 0

                n = 0
            

            if abs(borf1['_w00'][i] - borf2['_w00'][i]) >= 0.0000001 :

                print("\n -> borf1['_w00'][" + str(i) + "] != borf2['_w00'][" + str(i) + "]  " + " , " + str(borf1['_w00'][i]) + " != " + str(borf2['_w00'][i]))

                disc += "\n -> borf1['_w00'][" + str(i) + "] != borf2['_w00'][" + str(i) + "]  " + " , " + str(borf1['_w00'][i]) + " != " + str(borf2['_w00'][i])

                E = 0

                n = 0
            

            if abs(borf1['_Ax1'][i] - borf2['_Ax1'][i]) >= 0.0000001 :

                print("\n -> borf1['_Ax1'][" + str(i) + "] != borf2['_Ax1'][" + str(i) + "]  " + " , " + str(borf1['_Ax1'][i]) + " != " + str(borf2['_Ax1'][i]))

                disc += "\n -> borf1['_Ax1'][" + str(i) + "] != borf2['_Ax1'][" + str(i) + "]  " + " , " + str(borf1['_Ax1'][i]) + " != " + str(borf2['_Ax1'][i])

                E = 0

                n = 0
            

            if abs(borf1['_Ay1'][i] - borf2['_Ay1'][i]) >= 0.0000001 :

                print("\n -> borf1['_Ay1'][" + str(i) + "] != borf2['_Ay1'][" + str(i) + "]  " + " , " + str(borf1['_Ay1'][i]) + " != " + str(borf2['_Ay1'][i]))

                disc += "\n -> borf1['_Ay1'][" + str(i) + "] != borf2['_Ay1'][" + str(i) + "]  " + " , " + str(borf1['_Ay1'][i]) + " != " + str(borf2['_Ay1'][i])

                E = 0

                n = 0
            

            if abs(borf1['_Az1'][i] - borf2['_Az1'][i]) >= 0.0000001 :

                print("\n -> borf1['_Az1'][" + str(i) + "] != borf2['_Az1'][" + str(i) + "]  " + " , " + str(borf1['_Az1'][i]) + " != " + str(borf2['_Az1'][i]))

                disc += "\n -> borf1['_Az1'][" + str(i) + "] != borf2['_Az1'][" + str(i) + "]  " + " , " + str(borf1['_Az1'][i]) + " != " + str(borf2['_Az1'][i])

                E = 0

                n = 0
            

            if abs(borf1['_w01'][i] - borf2['_w01'][i]) >= 0.0000001 :

                print("\n -> borf1['_w01'][" + str(i) + "] != borf2['_w01'][" + str(i) + "]  " + " , " + str(borf1['_w01'][i]) + " != " + str(borf2['_w01'][i]))

                disc += "\n -> borf1['_w01'][" + str(i) + "] != borf2['_w01'][" + str(i) + "]  " + " , " + str(borf1['_w01'][i]) + " != " + str(borf2['_w01'][i])

                E = 0

                n = 0
            

            if abs(borf1['_Ax2'][i] - borf2['_Ax2'][i]) >= 0.0000001 :

                print("\n -> borf1['_Ax2'][" + str(i) + "] != borf2['_Ax2'][" + str(i) + "]  " + " , " + str(borf1['_Ax2'][i]) + " != " + str(borf2['_Ax2'][i]))

                disc += "\n -> borf1['_Ax2'][" + str(i) + "] != borf2['_Ax2'][" + str(i) + "]  " + " , " + str(borf1['_Ax2'][i]) + " != " + str(borf2['_Ax2'][i])

                E = 0

                n = 0
            

            if abs(borf1['_Ay2'][i] - borf2['_Ay2'][i]) >= 0.0000001 :

                print("\n -> borf1['_Ay2'][" + str(i) + "] != borf2['_Ay2'][" + str(i) + "]  " + " , " + str(borf1['_Ay2'][i]) + " != " + str(borf2['_Ay2'][i]))

                disc += "\n -> borf1['_Ay2'][" + str(i) + "] != borf2['_Ay2'][" + str(i) + "]  " + " , " + str(borf1['_Ay2'][i]) + " != " + str(borf2['_Ay2'][i])

                E = 0

                n = 0
            

            if abs(borf1['_Az2'][i] - borf2['_Az2'][i]) >= 0.0000001 :

                print("\n -> borf1['_Az2'][" + str(i) + "] != borf2['_Az2'][" + str(i) + "]  " + " , " + str(borf1['_Az2'][i]) + " != " + str(borf2['_Az2'][i]))

                disc += "\n -> borf1['_Az2'][" + str(i) + "] != borf2['_Az2'][" + str(i) + "]  " + " , " + str(borf1['_Az2'][i]) + " != " + str(borf2['_Az2'][i])

                E = 0

                n = 0
            

            if abs(borf1['_w02'][i] - borf2['_w02'][i]) >= 0.0000001 :

                print("\n -> borf1['_w02'][" + str(i) + "] != borf2['_w02'][" + str(i) + "]  " + " , " + str(borf1['_w02'][i]) + " != " + str(borf2['_w02'][i]))

                disc += "\n -> borf1['_w02'][" + str(i) + "] != borf2['_w02'][" + str(i) + "]  " + " , " + str(borf1['_w02'][i]) + " != " + str(borf2['_w02'][i])

                E = 0

                n = 0
            

            if (borf1['ID'][i] != borf2['ID'][i]) :

                print("\n -> borf1['ID'][" + str(i) + "] != borf2['ID'][" + str(i) + "]  " + " , " + str(borf1['ID'][i]) + " != " + str(borf2['ID'][i]))

                disc += "\n -> borf1['ID'][" + str(i) + "] != borf2['ID'][" + str(i) + "]  " + " , " + str(borf1['ID'][i]) + " != " + str(borf2['ID'][i])

                E = 0

                n = 0
            

            if (borf1['BoneID'][i] != borf2['BoneID'][i]) :

                print("\n -> borf1['BoneID'][" + str(i) + "] != borf2['BoneID'][" + str(i) + "]  " + " , " + str(borf1['BoneID'][i]) + " != " + str(borf2['BoneID'][i]))

                disc += "\n -> borf1['BoneID'][" + str(i) + "] != borf2['BoneID'][" + str(i) + "]  " + " , " + str(borf1['BoneID'][i]) + " != " + str(borf2['BoneID'][i])

                E = 0

                n = 0
            

            if (borf1['offset'][i] != borf2['offset'][i]) :

                print("\n -> borf1['offset'][" + str(i) + "] != borf2['offset'][" + str(i) + "]  " + " , " + str(borf1['offset'][i]) + " != " + str(borf2['offset'][i]))

                disc += "\n -> borf1['offset'][" + str(i) + "] != borf2['offset'][" + str(i) + "]  " + " , " + str(borf1['offset'][i]) + " != " + str(borf2['offset'][i])

                E = 0

                n = 0
            

            if (borf1['time'][i] != borf2['time'][i]) :

                print("\n -> borf1['time'][" + str(i) + "] != borf2['time'][" + str(i) + "]  " + " , " + str(borf1['time'][i]) + " != " + str(borf2['time'][i]))

                disc += "\n -> borf1['time'][" + str(i) + "] != borf2['time'][" + str(i) + "]  " + " , " + str(borf1['time'][i]) + " != " + str(borf2['time'][i])

                E = 0

                n = 0
            

            if (borf1['timer'][i] != borf2['timer'][i]) :

                print("\n -> borf1['timer'][" + str(i) + "] != borf2['timer'][" + str(i) + "]  " + " , " + str(borf1['timer'][i]) + " != " + str(borf2['timer'][i]))

                disc += "\n -> borf1['timer'][" + str(i) + "] != borf2['timer'][" + str(i) + "]  " + " , " + str(borf1['timer'][i]) + " != " + str(borf2['timer'][i])

                E = 0

                n = 0


    if n == 0 :

        Et += " 'BORF' "

# ------------------------>

    n = 1

    if ( len(botd1) != len(botd2) ):

        print("\n -> len(BOTD) != len(BOTD) ")

        disc += "\n -> len(BOTD) != len(BOTD) "

        E = 0

        n = 0

    else:

        for i in range ( 0 , len(botd1) ):
            

            if abs(botd1['px'][i] - botd2['px'][i]) >= 0.1 :

                print("\n -> botd1['px'][" + str(i) + "] != botd2['px'][" + str(i) + "]  " + " , " + str(botd1['px'][i]) + " != " + str(botd2['px'][i]))

                disc += "\n -> botd1['px'][" + str(i) + "] != botd2['px'][" + str(i) + "]  " + " , " + str(botd1['px'][i]) + " != " + str(botd2['px'][i])

                E = 0

                n = 0
            

            if abs(botd1['py'][i] - botd2['py'][i]) >= 0.1 :

                print("\n -> botd1['py'][" + str(i) + "] != botd2['py'][" + str(i) + "]  " + " , " + str(botd1['py'][i]) + " != " + str(botd2['py'][i]))

                disc += "\n -> botd1['py'][" + str(i) + "] != botd2['py'][" + str(i) + "]  " + " , " + str(botd1['py'][i]) + " != " + str(botd2['py'][i])

                E = 0

                n = 0
            

            if abs(botd1['pz'][i] - botd2['pz'][i]) >= 0.1 :

                print("\n -> botd1['pz'][" + str(i) + "] != botd2['pz'][" + str(i) + "]  " + " , " + str(botd1['pz'][i]) + " != " + str(botd2['pz'][i]))

                disc += "\n -> botd1['pz'][" + str(i) + "] != botd2['pz'][" + str(i) + "]  " + " , " + str(botd1['pz'][i]) + " != " + str(botd2['pz'][i])

                E = 0

                n = 0
            

            if (botd1['ID'][i] != botd2['ID'][i]) :

                print("\n -> botd1['ID'][" + str(i) + "] != botd2['ID'][" + str(i) + "]  " + " , " + str(botd1['ID'][i]) + " != " + str(botd2['ID'][i]))

                disc += "\n -> botd1['ID'][" + str(i) + "] != botd2['ID'][" + str(i) + "]  " + " , " + str(botd1['ID'][i]) + " != " + str(botd2['ID'][i])

                E = 0

                n = 0
            

            if (botd1['_0'][i] != botd2['_0'][i]) :

                print("\n -> botd1['_0'][" + str(i) + "] != botd2['_0'][" + str(i) + "]  " + " , " + str(botd1['_0'][i]) + " != " + str(botd2['_0'][i]))

                disc += "\n -> botd1['_0'][" + str(i) + "] != botd2['_0'][" + str(i) + "]  " + " , " + str(botd1['_0'][i]) + " != " + str(botd2['_0'][i])

                E = 0

                n = 0
            

            if (botd1['offset'][i] != botd2['offset'][i]) :

                print("\n -> botd1['offset'][" + str(i) + "] != botd2['offset'][" + str(i) + "]  " + " , " + str(botd1['offset'][i]) + " != " + str(botd2['offset'][i]))

                disc += "\n -> botd1['offset'][" + str(i) + "] != botd2['offset'][" + str(i) + "]  " + " , " + str(botd1['offset'][i]) + " != " + str(botd2['offset'][i])

                E = 0

                n = 0
            

            if (botd1['time'][i] != botd2['time'][i]) :

                print("\n -> botd1['time'][" + str(i) + "] != botd2['time'][" + str(i) + "]  " + " , " + str(botd1['time'][i]) + " != " + str(botd2['time'][i]))

                disc += "\n -> botd1['time'][" + str(i) + "] != botd2['time'][" + str(i) + "]  " + " , " + str(botd1['time'][i]) + " != " + str(botd2['time'][i])

                E = 0

                n = 0
            

            if (botd1['timer'][i] != botd2['timer'][i]) :

                print("\n -> botd1['timer'][" + str(i) + "] != botd2['timer'][" + str(i) + "]  " + " , " + str(botd1['timer'][i]) + " != " + str(botd2['timer'][i]))

                disc += "\n -> botd1['timer'][" + str(i) + "] != botd2['timer'][" + str(i) + "]  " + " , " + str(botd1['timer'][i]) + " != " + str(botd2['timer'][i])

                E = 0

                n = 0
            

            if (botd1['BoneID'][i] != botd2['BoneID'][i]) :

                print("\n -> botd1['BoneID'][" + str(i) + "] != botd2['BoneID'][" + str(i) + "]  " + " , " + str(botd1['BoneID'][i]) + " != " + str(botd2['BoneID'][i]))

                disc += "\n -> botd1['BoneID'][" + str(i) + "] != botd2['BoneID'][" + str(i) + "]  " + " , " + str(botd1['BoneID'][i]) + " != " + str(botd2['BoneID'][i])

                E = 0

                n = 0
            

            if (botd1['_id'][i] != botd2['_id'][i]) :

                print("\n -> botd1['_id'][" + str(i) + "] != botd2['_id'][" + str(i) + "]  " + " , " + str(botd1['_id'][i]) + " != " + str(botd2['_id'][i]))

                disc += "\n -> botd1['_id'][" + str(i) + "] != botd2['_id'][" + str(i) + "]  " + " , " + str(botd1['_id'][i]) + " != " + str(botd2['_id'][i])

                E = 0

                n = 0


    if n == 0 :

        Et += " 'BOTD' "

# ------------------------>

    n = 1

    if ( len(boaf1) != len(boaf2) ):

        print("\n -> len(BOAF) != len(BOAF) ")

        disc += "\n -> len(BOAF) != len(BOAF) "

        E = 0

        n = 0

    else:

        for i in range ( 0 , len(boaf1) ):
            

            if abs(boaf1['px'][i] - boaf2['px'][i]) >= 0.1 :

                print("\n -> boaf1['px'][" + str(i) + "] != boaf2['px'][" + str(i) + "]  " + " , " + str(boaf1['px'][i]) + " != " + str(boaf2['px'][i]))

                disc += "\n -> boaf1['px'][" + str(i) + "] != boaf2['px'][" + str(i) + "]  " + " , " + str(boaf1['px'][i]) + " != " + str(boaf2['px'][i])

                E = 0

                n = 0
            

            if abs(boaf1['py'][i] - boaf2['py'][i]) >= 0.1 :

                print("\n -> boaf1['py'][" + str(i) + "] != boaf2['py'][" + str(i) + "]  " + " , " + str(boaf1['py'][i]) + " != " + str(boaf2['py'][i]))

                disc += "\n -> boaf1['py'][" + str(i) + "] != boaf2['py'][" + str(i) + "]  " + " , " + str(boaf1['py'][i]) + " != " + str(boaf2['py'][i])

                E = 0

                n = 0
            

            if abs(boaf1['pz'][i] - boaf2['pz'][i]) >= 0.1 :

                print("\n -> boaf1['pz'][" + str(i) + "] != boaf2['pz'][" + str(i) + "]  " + " , " + str(boaf1['pz'][i]) + " != " + str(boaf2['pz'][i]))

                disc += "\n -> boaf1['pz'][" + str(i) + "] != boaf2['pz'][" + str(i) + "]  " + " , " + str(boaf1['pz'][i]) + " != " + str(boaf2['pz'][i])

                E = 0

                n = 0
            

            if (boaf1['ID'][i] != boaf2['ID'][i]) :

                print("\n -> boaf1['ID'][" + str(i) + "] != boaf2['ID'][" + str(i) + "]  " + " , " + str(boaf1['ID'][i]) + " != " + str(boaf2['ID'][i]))

                disc += "\n -> boaf1['ID'][" + str(i) + "] != boaf2['ID'][" + str(i) + "]  " + " , " + str(boaf1['ID'][i]) + " != " + str(boaf2['ID'][i])

                E = 0

                n = 0
            

            if (boaf1['_an'][i] != boaf2['_an'][i]) :

                print("\n -> boaf1['_an'][" + str(i) + "] != boaf2['_an'][" + str(i) + "]  " + " , " + str(boaf1['_an'][i]) + " != " + str(boaf2['_an'][i]))

                disc += "\n -> boaf1['_an'][" + str(i) + "] != boaf2['_an'][" + str(i) + "]  " + " , " + str(boaf1['_an'][i]) + " != " + str(boaf2['_an'][i])

                E = 0

                n = 0
            

            if (boaf1['offset'][i] != boaf2['offset'][i]) :

                print("\n -> boaf1['offset'][" + str(i) + "] != boaf2['offset'][" + str(i) + "]  " + " , " + str(boaf1['offset'][i]) + " != " + str(boaf2['offset'][i]))

                disc += "\n -> boaf1['offset'][" + str(i) + "] != boaf2['offset'][" + str(i) + "]  " + " , " + str(boaf1['offset'][i]) + " != " + str(boaf2['offset'][i])

                E = 0

                n = 0
            

            if (boaf1['time'][i] != boaf2['time'][i]) :

                print("\n -> boaf1['time'][" + str(i) + "] != boaf2['time'][" + str(i) + "]  " + " , " + str(boaf1['time'][i]) + " != " + str(boaf2['time'][i]))

                disc += "\n -> boaf1['time'][" + str(i) + "] != boaf2['time'][" + str(i) + "]  " + " , " + str(boaf1['time'][i]) + " != " + str(boaf2['time'][i])

                E = 0

                n = 0
            

            if (boaf1['timer'][i] != boaf2['timer'][i]) :

                print("\n -> boaf1['timer'][" + str(i) + "] != boaf2['timer'][" + str(i) + "]  " + " , " + str(boaf1['timer'][i]) + " != " + str(boaf2['timer'][i]))

                disc += "\n -> boaf1['timer'][" + str(i) + "] != boaf2['timer'][" + str(i) + "]  " + " , " + str(boaf1['timer'][i]) + " != " + str(boaf2['timer'][i])

                E = 0

                n = 0
            

            if abs(boaf1['_Ax'][i] - boaf2['_Ax'][i]) >= 0.0000001 :

                print("\n -> boaf1['_Ax'][" + str(i) + "] != boaf2['_Ax'][" + str(i) + "]  " + " , " + str(boaf1['_Ax'][i]) + " != " + str(boaf2['_Ax'][i]))

                disc += "\n -> boaf1['_Ax'][" + str(i) + "] != boaf2['_Ax'][" + str(i) + "]  " + " , " + str(boaf1['_Ax'][i]) + " != " + str(boaf2['_Ax'][i])

                E = 0

                n = 0
            

            if abs(boaf1['_Ay'][i] - boaf2['_Ay'][i]) >= 0.0000001 :

                print("\n -> boaf1['_Ay'][" + str(i) + "] != boaf2['_Ay'][" + str(i) + "]  " + " , " + str(boaf1['_Ay'][i]) + " != " + str(boaf2['_Ay'][i]))

                disc += "\n -> boaf1['_Ay'][" + str(i) + "] != boaf2['_Ay'][" + str(i) + "]  " + " , " + str(boaf1['_Ay'][i]) + " != " + str(boaf2['_Ay'][i])

                E = 0

                n = 0
            

            if abs(boaf1['_Az'][i] - boaf2['_Az'][i]) >= 0.0000001 :

                print("\n -> boaf1['_Az'][" + str(i) + "] != boaf2['_Az'][" + str(i) + "]  " + " , " + str(boaf1['_Az'][i]) + " != " + str(boaf2['_Az'][i]))

                disc += "\n -> boaf1['_Az'][" + str(i) + "] != boaf2['_Az'][" + str(i) + "]  " + " , " + str(boaf1['_Az'][i]) + " != " + str(boaf2['_Az'][i])

                E = 0

                n = 0
            

            if abs(boaf1['_w0'][i] - boaf2['_w0'][i]) >= 0.0000001 :

                print("\n -> boaf1['_w0'][" + str(i) + "] != boaf2['_w0'][" + str(i) + "]  " + " , " + str(boaf1['_w0'][i]) + " != " + str(boaf2['_w0'][i]))

                disc += "\n -> boaf1['_w0'][" + str(i) + "] != boaf2['_w0'][" + str(i) + "]  " + " , " + str(boaf1['_w0'][i]) + " != " + str(boaf2['_w0'][i])

                E = 0

                n = 0


    if n == 0 :

        Et += " 'BOAF' "

# ----------------------------->

    n = 1

    if ( len(banm1) != len(banm2) ):

        print("\n -> len(BANM) != len(BANM) ")

        disc += "\n -> len(BANM) != len(BANM) "

        E = 0

        n = 0

    else:

        for i in range ( 0 , len(banm1) ):
            

            if (banm1['ID'][i] != banm2['ID'][i]) :

                print("\n -> banm1['ID'][" + str(i) + "] != banm2['ID'][" + str(i) + "]  " + " , " + str(banm1['ID'][i]) + " != " + str(banm2['ID'][i]))

                disc += "\n -> banm1['ID'][" + str(i) + "] != banm2['ID'][" + str(i) + "]  " + " , " + str(banm1['ID'][i]) + " != " + str(banm2['ID'][i])

                E = 0

                n = 0
            

            if (banm1['_0'][i] != banm2['_0'][i]) :

                print("\n -> banm1['_0'][" + str(i) + "] != banm2['_0'][" + str(i) + "]  " + " , " + str(banm1['_0'][i]) + " != " + str(banm2['_0'][i]))

                disc += "\n -> banm1['_0'][" + str(i) + "] != banm2['_0'][" + str(i) + "]  " + " , " + str(banm1['_0'][i]) + " != " + str(banm2['_0'][i])

                E = 0

                n = 0
            

            if (banm1['offset'][i] != banm2['offset'][i]) :

                print("\n -> banm1['offset'][" + str(i) + "] != banm2['offset'][" + str(i) + "]  " + " , " + str(banm1['offset'][i]) + " != " + str(banm2['offset'][i]))

                disc += "\n -> banm1['offset'][" + str(i) + "] != banm2['offset'][" + str(i) + "]  " + " , " + str(banm1['offset'][i]) + " != " + str(banm2['offset'][i])

                E = 0

                n = 0
            

            if (banm1['duration'][i] != banm2['duration'][i]) :

                print("\n -> banm1['duration'][" + str(i) + "] != banm2['duration'][" + str(i) + "]  " + " , " + str(banm1['duration'][i]) + " != " + str(banm2['duration'][i]))

                disc += "\n -> banm1['duration'][" + str(i) + "] != banm2['duration'][" + str(i) + "]  " + " , " + str(banm1['duration'][i]) + " != " + str(banm2['duration'][i])

                E = 0

                n = 0
            

            if (banm1['_00'][i] != banm2['_00'][i]) :

                print("\n -> banm1['_00'][" + str(i) + "] != banm2['_00'][" + str(i) + "]  " + " , " + str(banm1['_00'][i]) + " != " + str(banm2['_00'][i]))

                disc += "\n -> banm1['_00'][" + str(i) + "] != banm2['_00'][" + str(i) + "]  " + " , " + str(banm1['_00'][i]) + " != " + str(banm2['_00'][i])

                E = 0

                n = 0


    if n == 0 :

        Et += " 'BANM' "

# ----------------------------->

    n = 1

    if ( len(bole1) != len(bole2) ):

        print("\n -> len(BOLE) != len(BOLE) ")

        disc += "\n -> len(BOLE) != len(BOLE) "

        E = 0

        n = 0

    else:

        for i in range ( 0 , len(bole1) ):
            

            if abs(bole1['_px'][i] - bole2['_px'][i]) >= 0.1 :

                print("\n -> bole1['_px'][" + str(i) + "] != bole2['_px'][" + str(i) + "]  " + " , " + str(bole1['_px'][i]) + " != " + str(bole2['_px'][i]))

                disc += "\n -> bole1['_px'][" + str(i) + "] != bole2['_px'][" + str(i) + "]  " + " , " + str(bole1['_px'][i]) + " != " + str(bole2['_px'][i])

                E = 0

                n = 0
            

            if abs(bole1['_py'][i] - bole2['_py'][i]) >= 0.1 :

                print("\n -> bole1['_py'][" + str(i) + "] != bole2['_py'][" + str(i) + "]  " + " , " + str(bole1['_py'][i]) + " != " + str(bole2['_py'][i]))

                disc += "\n -> bole1['_py'][" + str(i) + "] != bole2['_py'][" + str(i) + "]  " + " , " + str(bole1['_py'][i]) + " != " + str(bole2['_py'][i])

                E = 0

                n = 0
            

            if abs(bole1['_pz'][i] - bole2['_pz'][i]) >= 0.1 :

                print("\n -> bole1['_pz'][" + str(i) + "] != bole2['_pz'][" + str(i) + "]  " + " , " + str(bole1['_pz'][i]) + " != " + str(bole2['_pz'][i]))

                disc += "\n -> bole1['_pz'][" + str(i) + "] != bole2['_pz'][" + str(i) + "]  " + " , " + str(bole1['_pz'][i]) + " != " + str(bole2['_pz'][i])

                E = 0

                n = 0
            

            if abs(bole1['_Ax0'][i] - bole2['_Ax0'][i]) >= 0.0000001 :

                print("\n -> bole1['_Ax0'][" + str(i) + "] != bole2['_Ax0'][" + str(i) + "]  " + " , " + str(bole1['_Ax0'][i]) + " != " + str(bole2['_Ax0'][i]))

                disc += "\n -> bole1['_Ax0'][" + str(i) + "] != bole2['_Ax0'][" + str(i) + "]  " + " , " + str(bole1['_Ax0'][i]) + " != " + str(bole2['_Ax0'][i])

                E = 0

                n = 0
            

            if abs(bole1['_Ay0'][i] - bole2['_Ay0'][i]) >= 0.0000001 :

                print("\n -> bole1['_Ay0'][" + str(i) + "] != bole2['_Ay0'][" + str(i) + "]  " + " , " + str(bole1['_Ay0'][i]) + " != " + str(bole2['_Ay0'][i]))

                disc += "\n -> bole1['_Ay0'][" + str(i) + "] != bole2['_Ay0'][" + str(i) + "]  " + " , " + str(bole1['_Ay0'][i]) + " != " + str(bole2['_Ay0'][i])

                E = 0

                n = 0
            

            if abs(bole1['_Az0'][i] - bole2['_Az0'][i]) >= 0.0000001 :

                print("\n -> bole1['_Az0'][" + str(i) + "] != bole2['_Az0'][" + str(i) + "]  " + " , " + str(bole1['_Az0'][i]) + " != " + str(bole2['_Az0'][i]))

                disc += "\n -> bole1['_Az0'][" + str(i) + "] != bole2['_Az0'][" + str(i) + "]  " + " , " + str(bole1['_Az0'][i]) + " != " + str(bole2['_Az0'][i])

                E = 0

                n = 0
            

            if abs(bole1['_Ax1'][i] - bole2['_Ax1'][i]) >= 0.0000001 :

                print("\n -> bole1['_Ax1'][" + str(i) + "] != bole2['_Ax1'][" + str(i) + "]  " + " , " + str(bole1['_Ax1'][i]) + " != " + str(bole2['_Ax1'][i]))

                disc += "\n -> bole1['_Ax1'][" + str(i) + "] != bole2['_Ax1'][" + str(i) + "]  " + " , " + str(bole1['_Ax1'][i]) + " != " + str(bole2['_Ax1'][i])

                E = 0

                n = 0
            

            if abs(bole1['_Ay1'][i] - bole2['_Ay1'][i]) >= 0.0000001 :

                print("\n -> bole1['_Ay1'][" + str(i) + "] != bole2['_Ay1'][" + str(i) + "]  " + " , " + str(bole1['_Ay1'][i]) + " != " + str(bole2['_Ay1'][i]))

                disc += "\n -> bole1['_Ay1'][" + str(i) + "] != bole2['_Ay1'][" + str(i) + "]  " + " , " + str(bole1['_Ay1'][i]) + " != " + str(bole2['_Ay1'][i])

                E = 0

                n = 0
            

            if abs(bole1['_Az1'][i] - bole2['_Az1'][i]) >= 0.0000001 :

                print("\n -> bole1['_Az1'][" + str(i) + "] != bole2['_Az1'][" + str(i) + "]  " + " , " + str(bole1['_Az1'][i]) + " != " + str(bole2['_Az1'][i]))

                disc += "\n -> bole1['_Az1'][" + str(i) + "] != bole2['_Az1'][" + str(i) + "]  " + " , " + str(bole1['_Az1'][i]) + " != " + str(bole2['_Az1'][i])

                E = 0

                n = 0
            

            if abs(bole1['_Ax2'][i] - bole2['_Ax2'][i]) >= 0.0000001 :

                print("\n -> bole1['_Ax2'][" + str(i) + "] != bole2['_Ax2'][" + str(i) + "]  " + " , " + str(bole1['_Ax2'][i]) + " != " + str(bole2['_Ax2'][i]))

                disc += "\n -> bole1['_Ax2'][" + str(i) + "] != bole2['_Ax2'][" + str(i) + "]  " + " , " + str(bole1['_Ax2'][i]) + " != " + str(bole2['_Ax2'][i])

                E = 0

                n = 0
            

            if abs(bole1['_Ay2'][i] - bole2['_Ay2'][i]) >= 0.0000001 :

                print("\n -> bole1['_Ay2'][" + str(i) + "] != bole2['_Ay2'][" + str(i) + "]  " + " , " + str(bole1['_Ay2'][i]) + " != " + str(bole2['_Ay2'][i]))

                disc += "\n -> bole1['_Ay2'][" + str(i) + "] != bole2['_Ay2'][" + str(i) + "]  " + " , " + str(bole1['_Ay2'][i]) + " != " + str(bole2['_Ay2'][i])

                E = 0

                n = 0
            

            if abs(bole1['_Az2'][i] - bole2['_Az2'][i]) >= 0.0000001 :

                print("\n -> bole1['_Az2'][" + str(i) + "] != bole2['_Az2'][" + str(i) + "]  " + " , " + str(bole1['_Az2'][i]) + " != " + str(bole2['_Az2'][i]))

                disc += "\n -> bole1['_Az2'][" + str(i) + "] != bole2['_Az2'][" + str(i) + "]  " + " , " + str(bole1['_Az2'][i]) + " != " + str(bole2['_Az2'][i])

                E = 0

                n = 0
            

            if (bole1['ID'][i] != bole2['ID'][i]) :

                print("\n -> bole1['ID'][" + str(i) + "] != bole2['ID'][" + str(i) + "]  " + " , " + str(bole1['ID'][i]) + " != " + str(bole2['ID'][i]))

                disc += "\n -> bole1['ID'][" + str(i) + "] != bole2['ID'][" + str(i) + "]  " + " , " + str(bole1['ID'][i]) + " != " + str(bole2['ID'][i])

                E = 0

                n = 0
            

            if (bole1['_an'][i] != bole2['_an'][i]) :

                print("\n -> bole1['_an'][" + str(i) + "] != bole2['_an'][" + str(i) + "]  " + " , " + str(bole1['_an'][i]) + " != " + str(bole2['_an'][i]))

                disc += "\n -> bole1['_an'][" + str(i) + "] != bole2['_an'][" + str(i) + "]  " + " , " + str(bole1['_an'][i]) + " != " + str(bole2['_an'][i])

                E = 0

                n = 0
            

            if (bole1['offset'][i] != bole2['offset'][i]) :

                print("\n -> bole1['offset'][" + str(i) + "] != bole2['offset'][" + str(i) + "]  " + " , " + str(bole1['offset'][i]) + " != " + str(bole2['offset'][i]))

                disc += "\n -> bole1['offset'][" + str(i) + "] != bole2['offset'][" + str(i) + "]  " + " , " + str(bole1['offset'][i]) + " != " + str(bole2['offset'][i])

                E = 0

                n = 0
            

            if (bole1['time'][i] != bole2['time'][i]) :

                print("\n -> bole1['time'][" + str(i) + "] != bole2['time'][" + str(i) + "]  " + " , " + str(bole1['time'][i]) + " != " + str(bole2['time'][i]))

                disc += "\n -> bole1['time'][" + str(i) + "] != bole2['time'][" + str(i) + "]  " + " , " + str(bole1['time'][i]) + " != " + str(bole2['time'][i])

                E = 0

                n = 0
            

            if (bole1['timer'][i] != bole2['timer'][i]) :

                print("\n -> bole1['timer'][" + str(i) + "] != bole2['timer'][" + str(i) + "]  " + " , " + str(bole1['timer'][i]) + " != " + str(bole2['timer'][i]))

                disc += "\n -> bole1['timer'][" + str(i) + "] != bole2['timer'][" + str(i) + "]  " + " , " + str(bole1['timer'][i]) + " != " + str(bole2['timer'][i])

                E = 0

                n = 0
            

            if (bole1['BoneID'][i] != bole2['BoneID'][i]) :

                print("\n -> bole1['BoneID'][" + str(i) + "] != bole2['BoneID'][" + str(i) + "]  " + " , " + str(bole1['BoneID'][i]) + " != " + str(bole2['BoneID'][i]))

                disc += "\n -> bole1['BoneID'][" + str(i) + "] != bole2['BoneID'][" + str(i) + "]  " + " , " + str(bole1['BoneID'][i]) + " != " + str(bole2['BoneID'][i])

                E = 0

                n = 0


    if n == 0 :

        Et += " 'BOLE' "
        

#------------------------------------------------

    n = 1

    if ( len(atta1) != len(atta2) ):

        print("\n -> len(ATTA) != len(ATTA) ")

        disc += "\n -> len(ATTA) != len(ATTA) "

        E = 0

        n = 0

    else:

        for i in range ( 0 , len(atta1) ):
            
            if atta1['model_name'][i] != atta2['model_name'][i] :

                print("\n -> atta['model_name'][" + str(i) + "] != atta['model_name'][" + str(i) + "]  " + " , " + str(atta1['model_name'][i]) + " != " + str(atta2['model_name'][i]))

                disc += "\n -> atta['model_name'][" + str(i) + "] != atta['model_name'][" + str(i) + "]  " + " , " + str(atta1['model_name'][i]) + " != " + str(atta2['model_name'][i])

                E = 0

                n = 0

            if abs(atta1['px'][i] - atta2['px'][i]) >= 1 :

                print("\n -> atta['px'][" + str(i) + "] != atta['px'][" + str(i) + "]  " + " , " + str(atta1['px'][i]) + " != " + str(atta2['px'][i]))

                disc += "\n -> atta['px'][" + str(i) + "] != atta['px'][" + str(i) + "]  " + " , " + str(atta1['px'][i]) + " != " + str(atta2['px'][i])

                E = 0

                n = 0

            if abs(atta1['py'][i] - atta2['py'][i]) >= 1 :

                print("\n -> atta['py'][" + str(i) + "] != atta['py'][" + str(i) + "]  " + " , " + str(atta1['py'][i]) + " != " + str(atta2['py'][i]))

                disc += "\n -> atta['py'][" + str(i) + "] != atta['py'][" + str(i) + "]  " + " , " + str(atta1['py'][i]) + " != " + str(atta2['py'][i])

                E = 0

                n = 0

            if abs(atta1['pz'][i] - atta2['pz'][i]) >= 1 :

                print("\n -> atta['pz'][" + str(i) + "] != atta['pz'][" + str(i) + "]  " + " , " + str(atta1['pz'][i]) + " != " + str(atta2['pz'][i]))

                disc += "\n -> atta['pz'][" + str(i) + "] != atta['pz'][" + str(i) + "]  " + " , " + str(atta1['pz'][i]) + " != " + str(atta2['pz'][i])

                E = 0

                n = 0

            if atta1['_00'][i] != atta2['_00'][i] :

                print("\n -> atta['_00'][" + str(i) + "] != atta['_00'][" + str(i) + "]  " + " , " + str(atta1['_00'][i]) + " != " + str(atta2['_00'][i]))

                disc += "\n -> atta['_00'][" + str(i) + "] != atta['_00'][" + str(i) + "]  " + " , " + str(atta1['_00'][i]) + " != " + str(atta2['_00'][i])

                E = 0

                n = 0

            if atta1['_01'][i] != atta2['_01'][i] :

                print("\n -> atta['_01'][" + str(i) + "] != atta['_01'][" + str(i) + "]  " + " , " + str(atta1['_01'][i]) + " != " + str(atta2['_01'][i]))

                disc += "\n -> atta['_01'][" + str(i) + "] != atta['_01'][" + str(i) + "]  " + " , " + str(atta1['_01'][i]) + " != " + str(atta2['_01'][i])

                E = 0

                n = 0

            if atta1['_02'][i] != atta2['_02'][i] :

                print("\n -> atta['_02'][" + str(i) + "] != atta['_02'][" + str(i) + "]  " + " , " + str(atta1['_02'][i]) + " != " + str(atta2['_02'][i]))

                disc += "\n -> atta['_02'][" + str(i) + "] != atta['_02'][" + str(i) + "]  " + " , " + str(atta1['_02'][i]) + " != " + str(atta2['_02'][i])

                E = 0

                n = 0

            if atta1['_03'][i] != atta2['_03'][i] :

                print("\n -> atta['_03'][" + str(i) + "] != atta['_03'][" + str(i) + "]  " + " , " + str(atta1['_03'][i]) + " != " + str(atta2['_03'][i]))

                disc += "\n -> atta['_03'][" + str(i) + "] != atta['_03'][" + str(i) + "]  " + " , " + str(atta1['_03'][i]) + " != " + str(atta2['_03'][i])

                E = 0

                n = 0

            if atta1['_04'][i] != atta2['_04'][i] :

                print("\n -> atta['_04'][" + str(i) + "] != atta['_04'][" + str(i) + "]  " + " , " + str(atta1['_04'][i]) + " != " + str(atta2['_04'][i]))

                disc += "\n -> atta['_04'][" + str(i) + "] != atta['_04'][" + str(i) + "]  " + " , " + str(atta1['_04'][i]) + " != " + str(atta2['_04'][i])

                E = 0

                n = 0

            if atta1['_05'][i] != atta2['_05'][i] :

                print("\n -> atta['_05'][" + str(i) + "] != atta['_05'][" + str(i) + "]  " + " , " + str(atta1['_05'][i]) + " != " + str(atta2['_05'][i]))

                disc += "\n -> atta['_05'][" + str(i) + "] != atta['_05'][" + str(i) + "]  " + " , " + str(atta1['_05'][i]) + " != " + str(atta2['_05'][i])

                E = 0

                n = 0     

            if atta1['_06'][i] != atta2['_06'][i] :

                print("\n -> atta['_06'][" + str(i) + "] != atta['_06'][" + str(i) + "]  " + " , " + str(atta1['_06'][i]) + " != " + str(atta2['_06'][i]))

                disc += "\n -> atta['_06'][" + str(i) + "] != atta['_06'][" + str(i) + "]  " + " , " + str(atta1['_06'][i]) + " != " + str(atta2['_06'][i])

                E = 0

                n = 0    

            if atta1['_07'][i] != atta2['_07'][i] :

                print("\n -> atta['_07'][" + str(i) + "] != atta['_07'][" + str(i) + "]  " + " , " + str(atta1['_07'][i]) + " != " + str(atta2['_07'][i]))

                disc += "\n -> atta['_07'][" + str(i) + "] != atta['_07'][" + str(i) + "]  " + " , " + str(atta1['_07'][i]) + " != " + str(atta2['_07'][i])

                E = 0

                n = 0

            if atta1['_08'][i] != atta2['_08'][i] :

                print("\n -> atta['_08'][" + str(i) + "] != atta['_08'][" + str(i) + "]  " + " , " + str(atta1['_08'][i]) + " != " + str(atta2['_08'][i]))

                disc += "\n -> atta['_08'][" + str(i) + "] != atta['_08'][" + str(i) + "]  " + " , " + str(atta1['_08'][i]) + " != " + str(atta2['_08'][i])

                E = 0

                n = 0

            if atta1['_r0'][i] != atta2['_r0'][i] :

                print("\n -> atta['_r0'][" + str(i) + "] != atta['_r0'][" + str(i) + "]  " + " , " + str(atta1['_r0'][i]) + " != " + str(atta2['_r0'][i]))

                disc += "\n -> atta['_r0'][" + str(i) + "] != atta['_r0'][" + str(i) + "]  " + " , " + str(atta1['_r0'][i]) + " != " + str(atta2['_r0'][i])

                E = 0

                n = 0

            if atta1['_a'][i] != atta2['_a'][i] :

                print("\n -> atta['_a'][" + str(i) + "] != atta['_a'][" + str(i) + "]  " + " , " + str(atta1['_a'][i]) + " != " + str(atta2['_a'][i]))

                disc += "\n -> atta['_a'][" + str(i) + "] != atta['_a'][" + str(i) + "]  " + " , " + str(atta1['_a'][i]) + " != " + str(atta2['_a'][i])

                E = 0

                n = 0

            if atta1['_n'][i] != atta2['_n'][i] :

                print("\n -> atta['_n'][" + str(i) + "] != atta['_n'][" + str(i) + "]  " + " , " + str(atta1['_n'][i]) + " != " + str(atta2['_n'][i]))

                disc += "\n -> atta['_n'][" + str(i) + "] != atta['_n'][" + str(i) + "]  " + " , " + str(atta1['_n'][i]) + " != " + str(atta2['_n'][i])

                E = 0

                n = 0

            if atta1['_0'][i] != atta2['_0'][i] :

                print("\n -> atta['_0'][" + str(i) + "] != atta['_0'][" + str(i) + "]  " + " , " + str(atta1['_0'][i]) + " != " + str(atta2['_0'][i]))

                disc += "\n -> atta['_0'][" + str(i) + "] != atta['_0'][" + str(i) + "]  " + " , " + str(atta1['_0'][i]) + " != " + str(atta2['_0'][i])

                E = 0

                n = 0

    if n == 0 :

        Et += " 'ATTA' "


#------------------------------------------------

    return Et,disc

            
