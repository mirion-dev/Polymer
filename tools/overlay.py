import os
import pefile
import struct
import sys


def log(message: str):
    print(f'[overlay.py]: {message}')


(_, exe_path, patches_dir) = sys.argv

patcher_path = 'hpatchz.exe'
patcher_size = os.path.getsize(patcher_path)
log('Found the patcher.')

patches_path = []
patches_size = []
for name in os.listdir(patches_dir):
    path = os.path.join(patches_dir, name)
    if os.path.isfile(path):
        patches_path.append(path)
        patches_size.append(os.path.getsize(path))
log(f'Found {len(patches_path)} patches.')

with pefile.PE(exe_path) as pe:
    offset = pe.get_overlay_data_start_offset()

with open(exe_path, 'r+b') as exe:
    if offset is not None:
        exe.truncate(offset)
    exe.seek(0, 2)

    def write_name(path: str):
        name = os.path.basename(path)
        exe.write(struct.pack('H', len(name)))
        exe.write(name.encode('utf_16_le'))

    def write_offset(loc: int):
        current = exe.tell()
        exe.seek(loc)
        exe.write(struct.pack('I', current))
        exe.seek(current)

    exe.write(b'PLM\x00\x01\x00')
    log('Succeeded to write the signature.')

    patcher_loc = exe.tell()
    exe.write(struct.pack('2I', 0, patcher_size))
    write_name(patcher_path)
    log('Succeeded to write the patcher\'s metadata.')

    patches_loc = []
    exe.write(struct.pack('I', len(patches_path)))
    for (size, path) in zip(patches_size, patches_path):
        patches_loc.append(exe.tell())
        exe.write(struct.pack('2I', 0, size))
        write_name(path)
    log('Succeeded to write patches\' metadata.')

    with open(patcher_path, 'rb') as patcher:
        write_offset(patcher_loc)
        exe.write(patcher.read())
    log('Succeeded to write the patcher.')

    for (path, loc) in zip(patches_path, patches_loc):
        with open(path, 'rb') as patch:
            write_offset(loc)
            exe.write(patch.read())
    log('Succeeded to write patches.')
