import enum
import struct
import typing


class ByteOrder(enum.Enum):
    little = "<"
    big = ">"


class BinaryReader:
    def __init__(self, stream: bytes, byte_order: ByteOrder = ByteOrder.little):
        self.stream = stream
        self.byte_order = byte_order
        self._position = 0

    @property
    def position(self) -> int:
        return self._position
    
    def seek(self, offset: int, *, relative: bool = False):
        if relative:
            self._position += offset
        else:
            self._position = offset
        
    def seek_rel(self, offset: int):
        self.seek(offset, relative=True)

    @staticmethod
    def _check_len(buffer: bytes, size: int):
        if len(buffer) != size:
            raise OSError("EOF reached")

    def eof(self) -> bool:
        return self._position >= len(self.stream)
    
    def read(self, size: int = -1, suppress: bool = False) -> bytes:
        if size == -1:
            out = self.stream[self._position:]
            self._position += len(out)
            return out
        
        out = self.stream[self._position:self._position+size]
        self._position += size
        
        if not suppress:
            self._check_len(out, size)
        return out
    
    def _read(self, size: int, fmt: str, type_: type) -> typing.Any:
        out = self.read(size)
        endianness = self.byte_order.value
        return type_(*struct.unpack(endianness + fmt, out))

    def peek(self, size: int = 1) -> bytes:
        offset = self.position
        out = self.read(size)
        self.seek(offset)
        return out
    
    def read_bool(self) -> bool:
        out = self.read(1)
        if out == b'\x00':
            return False
        else:
            return True
        
    def read_s8(self) -> int:
        return self._read(1, "b", int)

    def read_u8(self) -> int:
        return self._read(1, "B", int)
        
    def read_s16(self) -> int:
        return self._read(2, "h", int)

    def read_u16(self) -> int:
        return self._read(2, "H", int)
    
    def read_u24(self) -> int:
        out = self.read(3)
        if self.byte_order == ByteOrder.little:
            return struct.unpack("<I", out + b'\x00')[0]
        else:
            return struct.unpack(">I", b'\x00' + out)[0]
        
    def read_s32(self) -> int:
        return self._read(4, "i", int)

    def read_u32(self) -> int:
        return self._read(4, "I", int)
        
    def read_s64(self) -> int:
        return self._read(8, "q", int)

    def read_u64(self) -> int:
        return self._read(8, "Q", int)
        
    def read_bytes(self, size: int) -> bytes:
        return self.read(size)
    
    def read_f16(self) -> float:
        return self._read(2, "s", float)
        
    def read_f32(self) -> float:
        return self._read(4, "f", float)
    
    def read_f64(self) -> float:
        return self._read(8, "d", float)
    
    def read_bools(self, count: int) -> list[bool]:
        return [self.read_bool() for _ in range(count)]
        
    def read_s8s(self, count: int) -> list[int]:
        return [self.read_s8() for _ in range(count)]

    def read_u8s(self, count: int) -> list[int]:
        return [self.read_u8() for _ in range(count)]
        
    def read_s16s(self, count: int) -> list[int]:
        return [self.read_s16() for _ in range(count)]

    def read_u16s(self, count: int) -> list[int]:
        return [self.read_u16() for _ in range(count)]
        
    def read_s32s(self, count: int) -> list[int]:
        return [self.read_s32() for _ in range(count)]

    def read_u32s(self, count: int) -> list[int]:
        return [self.read_u32() for _ in range(count)]
        
    def read_s64s(self, count: int) -> list[int]:
        return [self.read_s64() for _ in range(count)]

    def read_u64s(self, count: int) -> list[int]:
        return [self.read_u64() for _ in range(count)]

    def read_f16s(self, count: int) -> list[float]:
        return [self.read_f16() for _ in range(count)]

    def read_f32s(self, count: int) -> list[float]:
        return [self.read_f32() for _ in range(count)]

    def read_f64s(self, count: int) -> list[float]:
        return [self.read_f64() for _ in range(count)]

    def read_string(self, encoding_name: str, size: int = -1, char_size: int = 1) -> str:
        if size == -1:
            out = b""
            while (char := self.read(char_size)) != b'\x00':
                out += char
        else:
            out = self.read(size, suppress=True)
            # cut string off early at null terminator
            chars = [out[i:i+char_size] for i in range(0, size, char_size)]
            for i, char in enumerate(chars):
                if not int.from_bytes(char, self.byte_order.name):
                    out = out[:i]
        
        return out.decode(encoding_name)
    
    def read_signature(self, size: int, expected: str) -> str:
        signature = self.read_string("ascii", size)
        if signature != expected:
            Log.error(f"file signature was {signature}, expected {expected}")
        
        return signature
    
    def read_byte_order(self) -> ByteOrder:
        byte_order = self.read_bytes(2)

        if byte_order == b'\xFE\xFF':
            self.byte_order = ByteOrder.big
        elif byte_order == b'\xFF\xFE':
            self.byte_order = ByteOrder.little
        else:
            Log.error("invalid byte order")
        
        return self.byte_order
    
    def align(self, alignment: int):
        pos = self.position
        delta = (-pos % alignment + alignment) % alignment
        self.seek(delta, relative=True)


class BinaryWriter:
    def __init__(self, size: int = 0, byte_order: ByteOrder = ByteOrder.little):
        self.stream = bytearray(size)
        self.byte_order = byte_order
        self._position = 0

    @property
    def position(self) -> int:
        return self._position

    def save(self, filename: str):
        with open(filename, "wb") as f:
            f.write(self.stream)

    def seek(self, offset: int, *, relative: bool = False):
        if relative:
            self._position += offset
        else:
            self._position = offset
        
        self._fill_bytes(0)

    def align(self, alignment: int):
        pos = self.position
        delta = (-pos % alignment + alignment) % alignment
        self.seek(delta, relative=True)

    def _fill_bytes(self, offset: int, relative: bool = True):
        bytes_to_add = offset - len(self.stream)
        if relative:
            bytes_to_add += self.position

        if bytes_to_add > 0:
            self.stream += bytearray(bytes_to_add)

    def write(self, raw: bytes):
        self._fill_bytes(len(raw))
        for byte in raw:
            self.stream[self._position] = byte
            self._position += 1

    def _write(self, fmt: str, value):
        endianness = self.byte_order.value
        raw = struct.pack(endianness + fmt, value)
        self.write(raw)

    def write_bool(self, value: bool):
        self._write("?", value)

    def write_s8(self, value: int):
        self._write("b", value)

    def write_u8(self, value: int):
        self._write("B", value)

    def write_s16(self, value: int):
        self._write("h", value)

    def write_u16(self, value: int):
        self._write("H", value)

    def write_u24(self, value: int):
        if self.byte_order == ByteOrder.little:
            self.write(struct.pack("<I", value)[:3])
        else:
            self.write(struct.pack(">I", value)[1:])

    def write_s32(self, value: int):
        self._write("i", value)

    def write_u32(self, value: int):
        self._write("I", value)

    def write_s64(self, value: int):
        self._write("q", value)

    def write_u64(self, value: int):
        self._write("Q", value)

    def write_f32(self, value: float):
        self._write("f", value)

    def write_f64(self, value: float):
        self._write("d", value)

    def write_bytes(self, value: bytes):
        self.write(value)


class Vec3f:
    def __init__(self, x: float = 0.0, y: float = 0.0, z: float = 0.0):
        self.x = x
        self.y = y
        self.z = z
    
    def __repr__(self):
        return f"({self.x}, {self.y}, {self.z})"
    
    def __add__(self, other):
        return Vec3f(self.x + other.x, self.y + other.y, self.z + other.z)
    
    def __sub__(self, other):
        return Vec3f(self.x - other.x, self.y - other.y, self.z - other.z)
    
    def __mul__(self, other: float):
        return Vec3f(self.x * other, self.y * other, self.z * other)
    
    def __truediv__(self, other: float):
        return Vec3f(self.x / other, self.y / other, self.z / other)
    
    def __eq__(self, other):
        return all((self.x == other.x, self.y == other.y, self.z == other.z))
    
    def __iter__(self):
        return iter((self.x, self.y, self.z))
    
    def __hash__(self):
        return hash((self.x, self.y, self.z))

    def dot(self, other):
        return self.x * other.x + self.y * other.y + self.z * other.z
    
    def cross(self, other):
        return Vec3f(self.y * other.z - self.z * other.y,
                    self.z * other.x - self.x * other.z,
                    self.x * other.y - self.y * other.x)
    
    def mag(self):
        return math.sqrt(self.x * self.x + self.y * self.y + self.z * self.z)

    def normalized(self):
        return self / self.mag()
    
    @staticmethod
    def from_dict(d: dict[str, float]):
        return Vec3f(d.get('X', 0), d.get('Y', 0), d.get('Z', 0))
