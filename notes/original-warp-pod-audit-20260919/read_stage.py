"""Read-only inventory of authored BCSV metadata from the extracted stage archive."""
import json
import pathlib
import struct
import sys

data = pathlib.Path(sys.argv[1]).read_bytes()
if data[:4] == b'Yaz0':
    expected = struct.unpack_from('>I', data, 4)[0]
    out = bytearray()
    pos = 16
    while len(out) < expected:
        flags = data[pos]
        pos += 1
        for bit in range(7, -1, -1):
            if len(out) == expected:
                break
            if flags & (1 << bit):
                out.append(data[pos])
                pos += 1
            else:
                first, second = data[pos:pos + 2]
                pos += 2
                count = first >> 4
                if count == 0:
                    count = data[pos] + 0x12
                    pos += 1
                else:
                    count += 2
                distance = ((first & 15) << 8 | second) + 1
                for _ in range(count):
                    out.append(out[-distance])
    data = bytes(out)
assert data[:4] == b'RARC'
u32 = lambda offset: struct.unpack_from('>I', data, offset)[0]
u16 = lambda offset: struct.unpack_from('>H', data, offset)[0]
nodes, entries, strings, payload = [0x20 + u32(o) for o in (0x24, 0x2c, 0x34, 0x0c)]

fields = ['name', 'l_id', 'ParentID', 'GroupId', 'CastId', 'DemoGroupId',
          'SW_A', 'SW_B', 'SW_APPEAR', 'SW_DEAD'] + [f'Obj_arg{i}' for i in range(8)]
def hash_name(name):
    value = 0
    for c in name.encode():
        value = (value * 31 + c) & 0xffffffff
    return value
names = {hash_name(name): name for name in fields}
result = []
def table(path, blob):
    count, field_count, row_offset, row_size = struct.unpack_from('>4I', blob)
    info = [struct.unpack_from('>IIHBB', blob, 16 + i * 12) for i in range(field_count)]
    string_start = row_offset + count * row_size
    for row in range(count):
        values = {'table': path, 'row': row}
        for key, mask, offset, shift, kind in info:
            if key not in names:
                continue
            at = row_offset + row * row_size + offset
            if kind == 6:
                at = string_start + struct.unpack_from('>I', blob, at)[0]
                value = blob[at:blob.index(0, at)].decode('cp932')
            elif kind in (0, 3, 4, 5):
                width = {0: 4, 3: 4, 4: 2, 5: 1}[kind]
                value = (int.from_bytes(blob[at:at + width], 'big') & mask) >> shift
                if shift == 0 and mask == (1 << (width * 8)) - 1 and value >= (1 << (width * 8 - 1)):
                    value -= 1 << (width * 8)
            else:
                continue
            values[names[key]] = value
        if values.get('name') in ('WarpPod', 'EarthenPipe', 'RunawayRabbitCollect', 'RunawayRabbit', 'RunawayTico', 'SwitchCube'):
            result.append(values)

def walk(node, path):
    offset = nodes + node * 16
    for i in range(u16(offset + 10)):
        entry = entries + (u32(offset + 12) + i) * 20
        meta = u32(entry + 4)
        name_at = strings + (meta & 0xffffff)
        name = data[name_at:data.index(0, name_at)].decode('cp932')
        if name in ('.', '..'):
            continue
        current = path + '/' + name
        if meta >> 24 & 2:
            walk(u32(entry + 8), current)
        elif name in ('objinfo', 'childobjinfo', 'areaobjinfo'):
            at = payload + u32(entry + 8)
            table(current, data[at:at + u32(entry + 12)])
walk(0, '')
print(json.dumps(result, ensure_ascii=False, indent=2))
