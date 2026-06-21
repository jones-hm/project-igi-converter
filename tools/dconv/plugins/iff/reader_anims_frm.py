import os
import io
import struct
from .import reader_anims_data
from .reader_anims_data import *

def S(Size):
    B = struct.pack('=1I',Size)
    Sz = struct.pack('=4B',B[3],B[2],B[1],B[0])
    return struct.unpack('=1I',Sz)[0]


def GETDATA(POS,File,Size,Pth,X):

    with open(File, 'rb') as FILE:
        
        FILE.seek(POS,0)
        

# Reading Anim Header -->


        Temp = struct.unpack('=4s1I4s', FILE.read(12))

        Chunk_sign = (Temp[0])
        Chunk_Size = S(Temp[1])

        assert Chunk_sign == b'FORM', "signature must be b'FORM'"
        assert Chunk_sign != b'BOAN', "signature must be b'BOAN'"
        

# Reading Anim Data -->


        Chunks = []

        Pos = FILE.tell()-4

        while ( Pos-FILE.tell() != - Chunk_Size ):

            temp = struct.unpack('=4s1I', FILE.read(8))

            chunk_sign = (temp[0])
            chunk_size = S(temp[1])

            read_bytes = FILE.read(chunk_size)

            Chunks.append([chunk_sign,chunk_size,chunk_size,4,read_bytes])

            if ( chunk_sign == b'BOAH' and X == 0 ):

             NameOFAnim = (struct.unpack('=1Q1I',read_bytes))[1]

             if NameOFAnim < 10 :
                                NameOFAnim = "00"+str(NameOFAnim)
             elif NameOFAnim < 100 :
                                NameOFAnim = "0"+str(NameOFAnim)
             elif NameOFAnim < 1000 :
                                NameOFAnim = ""+str(NameOFAnim)
                                
             print(" -> anim_"+NameOFAnim+".iFF")
            
        

# Writing Data -->
        

        if ( X == 0 ):

            Nam = (os.path.splitext(os.path.basename(File))[0])

            Pth += "\\"

            File2 = str(Pth)+"anims_"+str(Nam)+"\\"

            Dat = reader_anims_data.EXE(Chunks,File2)

            POS = FILE.tell()

            return POS,Dat

        else: # For read-only

            POS = FILE.tell()

            return POS,Chunks
