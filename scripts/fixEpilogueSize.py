import pefile.pefile as pefile
import argparse

def getVirtualAddress(pe_object,file_offset, section_name):
    pe = pefile.PE(pe_object)
    vaAddr = 0
    for section in pe.sections:
        if section.Name.decode('utf-8').rstrip('\x00') == section_name:
            vaAddr = section.get_rva_from_offset(file_offset)
            break
    pe.close()
    return vaAddr

def findEpilogueSize(filename, functionAddr):
    pe = pefile.PE(filename)
    epilogueSize = 0
    if pe.OPTIONAL_HEADER.DATA_DIRECTORY[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_EXCEPTION']].VirtualAddress != 0:
         pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_EXCEPTION']])
         if pe.DIRECTORY_ENTRY_EXCEPTION is not None:
            for entry in pe.DIRECTORY_ENTRY_EXCEPTION:
                if functionAddr >= entry.struct.BeginAddress and functionAddr <= entry.struct.EndAddress:
                    for c in entry.unwindinfo.UnwindCodes:
                        if c.struct.UnwindOp == pefile.UWOP_PUSH_NONVOL:
                            epilogueSize += 8
                        elif c.struct.UnwindOp == pefile.UWOP_ALLOC_LARGE:
                            if c.struct.OpInfo == 0:
                                epilogueSize += c.struct.AllocSizeInQwords * 8
                            else: 
                                epilogueSize += c.struct.AllocSize
                        elif c.struct.UnwindOp == pefile.UWOP_ALLOC_SMALL:
                                epilogueSize += c.struct.AllocSizeInQwordsMinus8 * 8 + 8
                        else:
                            pe.close()
                            return -1
    pe.close()
    print("[-] Found pattern in function with epilogue size: ", hex(epilogueSize))
    return epilogueSize

def setEpilogueSize(filename, pattern):
    bytePattern = pattern.to_bytes(4, "little")
    binData = open(filename, 'rb+').read()
    virtualAddr = getVirtualAddress(filename, binData.find(bytePattern), ".text")
    epilogueSize = findEpilogueSize(filename, virtualAddr)
    if epilogueSize > 0:
        binData = binData.replace(bytePattern, epilogueSize.to_bytes(4, "little"))
        outFile = open(filename, 'wb+')
        outFile.write(binData)
        outFile.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-fd', '--filepath', help='Path to exe', required=True)
    args = parser.parse_args()
    setEpilogueSize(args.filepath, 0xFAFBFCFD)
    setEpilogueSize(args.filepath, 0xFBFBFAFA)