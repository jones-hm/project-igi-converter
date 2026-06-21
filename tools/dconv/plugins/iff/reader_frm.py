import os
import io
import struct
import builtins
from .import IGI1_iff
from .import reader_anims_frm
from .IGI1_iff import *
from .reader_anims_frm import *

def S(Size):
    B = struct.pack('=1I',Size)
    Sz = struct.pack('=4B',B[3],B[2],B[1],B[0])
    return struct.unpack('=1I',Sz)[0]

from collections import namedtuple
chunk_info_names = ('signature','size','align','skip','start','datapos')
ChunkInfo = namedtuple('ChunkInfo', chunk_info_names)


class FORMReader:
    def __init__(self, stream, File, Pth, X):
        self._stream = stream
        self._chunks = list()

        # seek to end of stream and save position (stream size)
        self._stream.seek(0, 2)
        size = self._stream.tell()

        # seek to begin of stream and try to read ilff header
        self._stream.seek(0, 0)
        
        Temp = struct.unpack('=4s1I4s', self._stream.read(12))

        self._signature = (Temp[0])
        self._HederSize = S(Temp[1])
        
        assert self._signature == b'FORM', "signature must be b'FORM'"
        assert self._signature != b'BOBJ', "signature must be b'BOBJ'"

        


 # Reading Bone Header Bytes ------------->
 
        
        temp = struct.unpack('=4s1I8s1I', self._stream.read(20))
            
        chunk_start = self._stream.tell()

        chunk_signature = (temp[0])
        chunk_ChunkSize = S(temp[1])
        chunk_ChunkSkip = S(temp[3])

        chunk_datapos   = self._stream.tell()

        self._stream.seek(self._stream.tell()+chunk_ChunkSkip, 0)

        self._chunks.append((self._signature,8,4,8,chunk_start,chunk_datapos))


 # Reading Bone Bytes ---->
 
 
        Pos = self._stream.tell()-20

        while ( Pos - self._stream.tell() != - chunk_ChunkSize ):
            
            chunk_start = self._stream.tell()

            temp = struct.unpack('=4s1I', self._stream.read(8))

            chunk_signature = (temp[0])
            chunk_size      = S(temp[1])

            chunk_datapos   = self._stream.tell()

            self._stream.seek(self._stream.tell()+chunk_size, 0)

            self._chunks.append((chunk_signature,chunk_size,4,chunk_size,chunk_start,chunk_datapos))

            


 # Reading Anims Data ------------------>
        

        Temp = struct.unpack('=4s1I8s1I', self._stream.read(20))
            
        chunk_start = self._stream.tell()

        chunk_signature = (Temp[0])
        chunk_ChunkSize = S(Temp[1])
        chunk_Hder_Sign = (Temp[2])
        chunk_ChunkSkip = S(Temp[3])

        chunk_datapos   = self._stream.tell()

        self._stream.seek(self._stream.tell()+chunk_ChunkSkip, 0)

        self._chunks.append((chunk_signature,8,4,8,chunk_start,chunk_datapos))


# File-By-File-->


        Names = []

        Pos = self._stream.tell()

        if ( X == 0 ): # FOR ANIMS DATA DECOMPILING ( X = 0 ) 

         while ( Pos <= ( chunk_ChunkSize + self._HederSize ) - 8 ):

          AnimData = reader_anims_frm.GETDATA(Pos,File,size,Pth,X)

          Pos = AnimData[0]

          self._stream.seek(Pos,0)

          Names.append(AnimData[1])

         IGI1_iff.Names = Names

         if ( (size-self._stream.tell()) != 0 ):

          assert 0 != 0 , "Expected End of File Not Found"


 

# Functions


    def close(self):
        self._stream.close()

    def __enter__(self):
        return self

    def tellsize(self):
        self._stream.seek(0, 2)
        size = self._stream.tell()
        return size

    def __exit__(self, *args):
        self.close()

    def __del__(self):
        self.close()

    def signatures(self):
        return [item[0] for item in self._chunks]

    def find(self, chunk_signature):
        return chunk_signature in self.signatures()

    def seek(self, chunk_signature, skipone=False):
        skiped = False

        for item in self._chunks:
            if item[0] == chunk_signature:
                if skipone:
                    if not skiped:
                        skiped = True
                        continue

                self._stream.seek(item[5])
                return item


    def read(self, chunk_signature, skipone=False):
        chunk_info = self.seek(chunk_signature, skipone)

        if chunk_info:
            return self._stream.read(chunk_info[1])


    def info(self, chunk_signature, skipone=False):
        skiped = False

        for item in self._chunks:
            if item[0] == chunk_signature:
                if skipone:
                    if not skiped:
                        skiped = True
                        continue

                return ChunkInfo(item)


def open(file,Pth,X, mode=None):
    if isinstance(file, str):
        return FORMReader(builtins.open(file, 'rb'),file,Pth,X)
    elif isinstance(file, io.BytesIO):
        return FORMReader(file,Pth,X)
    else:
        raise ValueError("file expected str or BytesIO")
