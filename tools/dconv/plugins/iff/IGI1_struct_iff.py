import numpy as np

DTYPE_FORM = np.dtype([
    ('_count_00', np.int32),
    ('_count_01', np.int32),
    ])

DTYPE_PLST = np.dtype([
    ('Id_child', np.int32),
    ])

DTYPE_TLST = np.dtype([
    ('px', np.float32),
    ('py', np.float32),
    ('pz', np.float32),
    ])

DTYPE_BOCD = np.dtype([
    ('count', np.uint32),
    ])

DTYPE_BOAH = np.dtype([
    ('length', np.float32),
    ('_00',    np.uint16),
    ('_01',    np.uint16),
    ('_id',    np.uint32),
    ])

DTYPE_BOTD = np.dtype([
    ('px', np.float32),
    ('py', np.float32),
    ('pz', np.float32),
    ('time', np.float32),
    ('_Ax0', np.float32),
    ('_Ay0', np.float32),
    ('_Az0', np.float32),
    ('_Ax1', np.float32),
    ('_Ay1', np.float32),
    ('_Az1', np.float32),
    ])

DTYPE_BORD = np.dtype([
    ('_Ax0',    np.float32),   
    ('_Ay0',    np.float32),
    ('_Az0',    np.float32),
    ('_w00',    np.float32),
    ('time',    np.float32),
    ('_Ax1',    np.float32),   
    ('_Ay1',    np.float32),
    ('_Az1',    np.float32),
    ('_w01',    np.float32),
    ('_Ax2',    np.float32),   
    ('_Ay2',    np.float32),
    ('_Az2',    np.float32),
    ('_w02',    np.float32),
    ])

DTYPE_BOED = np.dtype([
    ('_id',      np.int32),
    ('BoneID',   np.int32),
    ('time',     np.float32),
    ('px',       np.float32),
    ('py',       np.float32),
    ('pz',       np.float32),
    ])



def parse_form(form_bytes):
    return np.frombuffer(form_bytes, DTYPE_FORM)

def parse_plst(plst_bytes):
    return np.frombuffer(plst_bytes, DTYPE_PLST)

def parse_tlst(tlst_bytes):
    return np.frombuffer(tlst_bytes, DTYPE_TLST)

def parse_boah(boah_bytes):
    return np.frombuffer(boah_bytes, DTYPE_BOAH)

def parse_both(both_bytes):
    return np.frombuffer(both_bytes, DTYPE_BOCD)

def parse_borh(borh_bytes):
    return np.frombuffer(borh_bytes, DTYPE_BOCD)

def parse_boeh(boeh_bytes):
    return np.frombuffer(boeh_bytes, DTYPE_BOCD)

def parse_botd(botd_bytes):
    return np.frombuffer(botd_bytes, DTYPE_BOTD)

def parse_bord(bord_bytes):
    return np.frombuffer(bord_bytes, DTYPE_BORD)

def parse_boed(boed_bytes):
    return np.frombuffer(boed_bytes, DTYPE_BOED)
    
