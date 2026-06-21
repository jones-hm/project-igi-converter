import numpy as np
	
DTYPE_DHNA = np.dtype([
    ('r00_0',        [('r', np.uint32),]),   # Reserved
    ('r01_0',        [('r', np.uint32),]),   # Reserved
    ('r02_4',               np.uint32),      # 4
    ('anim_type',           np.uint32),      # 0 or 1 
    ('anim_duration',       np.uint32),      # Duration
    ('num_bones',           np.uint32),      # Bones
    ('r03_0',        [('r', np.uint32),]),   # Reserved
    ('num_frames',          np.uint32),      # Frames
    ('r04_0',               np.uint32),      # 0
    ('num_attach',          np.uint32),      # Attachments
    ('r05_0',               np.uint32),      # 0
    ('r06_0',        [('r', np.uint32),]),   # Reserved
    ('r07_0',        [('r', np.uint32),]),   # Reserved
    ('_00',                 np.uint8),       # buffer
    ])

DTYPE_REIH_P0 = np.dtype([
    ('num_child', np.uint8),
    ])

DTYPE_REIH_P1 = np.dtype([
    ('px', np.float32),
    ('py', np.float32),
    ('pz', np.float32),
    ])

DTYPE_TNVE_0 = np.dtype([
    ('checkID',  np.int8),
    ])

DTYPE_TNVE_1 = np.dtype([
    ('ID',      np.int8),
    ('_0',      np.int8),
    ('offset',  np.uint16),
    ('time',    np.uint32),
    ('timer',   np.uint32),
    ('px',      np.float32),
    ('py',      np.float32),
    ('pz',      np.float32),
    ])

DTYPE_TNVE_2 = np.dtype([
    ('ID',           np.uint8),
    ('BoneID',       np.uint8),
    ('offset',       np.uint16),   
    ('time',         np.uint32),
    ('timer',        np.uint32),  
    ('_Ax0',         np.float32),   
    ('_Ay0',         np.float32),
    ('_Az0',         np.float32),
    ('_w00',         np.float32),
    ('_rs0',   [('r', np.uint32), ]),        
    ('_Ax1',         np.float32),   
    ('_Ay1',         np.float32),
    ('_Az1',         np.float32),
    ('_w01',         np.float32),
    ('_rs1',  [('r', np.uint32), ]),        
    ('_Ax2',         np.float32),   
    ('_Ay2',         np.float32),
    ('_Az2',         np.float32),
    ('_w02',         np.float32),
    ('_rs2',  [('r', np.uint32), ]),         
    ])

DTYPE_TNVE_3 = np.dtype([
    ('ID',          np.uint8),
    ('_0',          np.uint8),
    ('offset',      np.uint16),    
    ('time',        np.uint32),
    ('timer',       np.uint32),
    ('BoneID',      np.int32),
    ('_id',         np.int16),
    ('_rs',  [('r', np.uint16), ]),    
    ('px',          np.float32),
    ('py',          np.float32),
    ('pz',          np.float32),     
    ])

DTYPE_TNVE_4 = np.dtype([
    ('ID',          np.uint8),
    ('_an',         np.uint8),    
    ('offset',      np.uint16),    
    ('time',        np.uint32),
    ('timer',       np.uint32),
    ('px',          np.float32),
    ('py',          np.float32),
    ('pz',          np.float32),
    ('_Ax',         np.float32),
    ('_Ay',         np.float32),
    ('_Az',         np.float32),
    ('_w0',         np.float32),
    ('_rs',  [('r', np.uint32), ]),      
    ])

DTYPE_TNVE_5 = np.dtype([
    ('ID',           np.uint8),
    ('_an',          np.uint8),
    ('offset',       np.uint16),
    ('time',         np.uint32),
    ('timer',        np.uint32),
    ('BoneID',       np.uint32),
    ('_Ax0',         np.float32),
    ('_Ay0',         np.float32),
    ('_Az0',         np.float32),
    ('_Ax1',         np.float32),
    ('_Ay1',         np.float32),
    ('_Az1',         np.float32),
    ('_Ax2',         np.float32),
    ('_Ay2',         np.float32),
    ('_Az2',         np.float32),
    ('_rs',   [('r', np.uint32), ]), 
    ('_px',          np.float32),
    ('_py',          np.float32),
    ('_pz',          np.float32),
    ])

DTYPE_TNVE_6 = np.dtype([
    ('ID',      np.int8),
    ('_0',      np.int8),
    ('offset',  np.uint16),
    ('duration',np.uint32),
    ('_00',     np.uint32),
    ])

DTYPE_ATTA = np.dtype([
    ('model_name', (np.bytes_, 16)),
    ('px',   np.float32),
    ('py',   np.float32),
    ('pz',   np.float32),
    ('_00',  np.float32),
    ('_01',  np.float32),
    ('_02',  np.float32),
    ('_03',  np.float32),
    ('_04',  np.float32),
    ('_05',  np.float32),
    ('_06',  np.float32),
    ('_07',  np.float32),
    ('_08',  np.float32),
    ('_r0',  np.uint32),    
    ('_a',   np.int32),
    ('_n',   np.uint32),        
    ('_0',   np.uint32),           
    ])

def parse_dhna(dhna_bytes):
    return np.frombuffer(dhna_bytes[:53], DTYPE_DHNA)
    
def parse_reih(reih_bytes):
    count = len(reih_bytes) // 13
    align = len(reih_bytes) % 13

    p1 = np.frombuffer(reih_bytes[:count], DTYPE_REIH_P0)
    p2 = np.frombuffer(reih_bytes[count+align:], DTYPE_REIH_P1)

    return p1, p2    

def parse_tnve(tnve_bytes):

    X = 0

    Tf = b''
    Rf = b''
    Af = b''
    Td = b''
    Le = b''
    Ed = b''

    while ( X != len(tnve_bytes) ):

        P = np.frombuffer(tnve_bytes[X:X+1],DTYPE_TNVE_0)

        if ( P['checkID'][0] == 1 ):
            
            Le += tnve_bytes[X:X+68]
            X = X + 68

        elif ( P['checkID'][0] == 3 ):

            Tf += tnve_bytes[X:X+24]
            X = X + 24

        elif ( P['checkID'][0] == 4 ):

            Rf += tnve_bytes[X:X+72]
            X = X + 72

        elif ( P['checkID'][0] == 6 ):

            Td += tnve_bytes[X:X+32]
            X = X + 32

        elif ( P['checkID'][0] == 7 ):

            Af += tnve_bytes[X:X+44]
            X = X + 44

        elif ( P['checkID'][0] == -1 ):

            Ed += tnve_bytes[X:X+12]
            X = X + 12

    Tf = np.frombuffer(Tf,   DTYPE_TNVE_1)

    Rf = np.frombuffer(Rf,   DTYPE_TNVE_2)

    Td = np.frombuffer(Td,   DTYPE_TNVE_3)

    Af = np.frombuffer(Af,   DTYPE_TNVE_4)

    Le = np.frombuffer(Le,   DTYPE_TNVE_5)

    Ed = np.frombuffer(Ed,   DTYPE_TNVE_6)
    
    return Tf,Rf,Td,Af,Le,Ed
    
def parse_atta(atta_bytes):
    return np.frombuffer(atta_bytes, DTYPE_ATTA)    
    

