import struct
import array
import numpy as np

from utils import ilff
	
DHNA = np.dtype([
    ('_0',  np.uint32),
    ('_1',  np.uint32),
    ('_2',  np.uint32),
    ('reserved_1',  (np.uint8, 4)),    
    ('bone_count',  np.uint32),    
    ('_3',  np.uint32),
    ('_4',  np.uint32),
    ('_5',  np.uint32),    
    ('_6',  np.uint32),        
    ('reserved_2',  (np.uint8, 8)),        
    ('Name_anim', (np.bytes_, 16)),
	])
	
REIH = np.dtype([
    ('hiearchy',  np.uint8),  
    ('_0',  np.uint32),      
    ('_1',  np.uint32),    
    ('_2',  np.uint32),    
    ('_3',  np.uint32),        
    ('px',  np.float32),
    ('py',  np.float32),
    ('pz',  np.float32),
	])
	
TNVE = np.dtype([
    ('_0',  np.float32),            
	])
	
ATTA = np.dtype([
    ('model_name', (np.bytes_, 16)),
    ('px',  np.float32),
    ('py',  np.float32),
    ('pz',  np.float32),
    ('_00',  np.float32),
    ('_01',  np.float32),
    ('_02',  np.float32),
    ('_03',  np.float32),
    ('_04',  np.float32),
    ('_05',  np.float32),
    ('_06',  np.float32),
    ('_07',  np.float32),
    ('_08',  np.float32),
    ('reserved_0', (np.bytes_, 4)),    
    ('_09',  np.int32),
    ('_10',  np.int32),        
    ('_11',  np.int32),           
	])	


class IFF:
	__slots__ = (
		'dhna',
		'reih',
		'tnve',
		'atta',
		)

	def __init__(self):
		self.dhna = list()
		self.reih = list()
		self.tnve = list()
		self.atta = list()

	def load(self, fp):
		if isinstance(fp, str):
			fp = open(fp, 'rb')

		ff = ilff.fromfile(fp)

		for chunk in ff.chunks():
			if chunk.fourcc == b'DHNA':
				self.dhna = np.frombuffer(chunk.read(), DHNA)

			elif chunk.fourcc == b'REIH':
				self.reih = np.frombuffer(chunk.read(), REIH)

			elif chunk.fourcc == b'TNVE':
				self.tnve = np.frombuffer(chunk.read(), TNVE)

			elif chunk.fourcc == b'ATTA':
				self.atta = np.frombuffer(chunk.read(), ATTA)

			else:
				raise ValueError()

			assert not chunk.read(), "Expect end of chunk"


		fp.close()

	def save(self, fp):
		NotImplemented
