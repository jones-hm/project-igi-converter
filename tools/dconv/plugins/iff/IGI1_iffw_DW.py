import os
from utils import fs

def WRITE(File,Data):

    os.makedirs(os.path.dirname(File), exist_ok=True)
        
    with open(File, 'wb') as F:
         F.write(Data)
         
    return "NONE"
