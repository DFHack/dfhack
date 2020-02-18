import ghidra.app.script.*;
import ghidra.program.model.address.*;
import ghidra.program.model.data.*;
import ghidra.program.model.mem.*;

public class find_df_globals extends GhidraScript {
    public void run() throws Exception {
        int ptrSize = currentProgram.getDefaultPointerSize();
        byte[] dfInit = new byte[ptrSize * 2];
        int i = 0;
        dfInit[i++] = 0x78;
        dfInit[i++] = 0x56;
        dfInit[i++] = 0x34;
        dfInit[i++] = 0x12;
        if (ptrSize >= 8) {
            dfInit[i++] = 0x78;
            dfInit[i++] = 0x56;
            dfInit[i++] = 0x34;
            dfInit[i++] = 0x12;
            dfInit[i++] = 0x21;
            dfInit[i++] = 0x43;
            dfInit[i++] = 0x65;
            dfInit[i++] = (byte)0x87;
        }
        dfInit[i++] = 0x21;
        dfInit[i++] = 0x43;
        dfInit[i++] = 0x65;
        dfInit[i++] = (byte)0x87;
        byte[] mask = new byte[ptrSize * 2];
        for (i = 0; i < ptrSize * 2; i++) {
            mask[i] = (byte)0xff;
        }

        DataTypeManager dtm = currentProgram.getDataTypeManager();
        StructureDataType dfGlobalsMapElement = new StructureDataType("df_globals_map_element", 0);
        dfGlobalsMapElement.add(new PointerDataType(CharDataType.dataType, ptrSize), "name", null);
        dfGlobalsMapElement.add(new PointerDataType(DataType.DEFAULT, ptrSize), "addr", null);
        StructureDataType dfGlobalsMap = new StructureDataType("df_globals_map", 0);
        dfGlobalsMap.add(DWordDataType.dataType, "magic0", "12345678");
        if (ptrSize < 8) {
            dfGlobalsMap.add(DWordDataType.dataType, "magic1", "87654321");
        } else {
            dfGlobalsMap.add(DWordDataType.dataType, "magic1", "12345678");
            dfGlobalsMap.add(DWordDataType.dataType, "magic2", "87654321");
            dfGlobalsMap.add(DWordDataType.dataType, "magic3", "87654321");
        }

        Memory mem = currentProgram.getMemory();
        Address globalAddr = mem.findBytes(currentProgram.getMinAddress(), dfInit, mask, true, monitor);

        int globalCount = 0;
        while (mem.getLong(globalAddr.add((globalCount + 1) * ptrSize * 2)) != 0) {
            globalCount++;
            Address nameAddr;
            Address dataAddr;
            if (ptrSize >= 8) {
                nameAddr = globalAddr.getNewAddress(mem.getLong(globalAddr.add(globalCount * ptrSize * 2)));
                dataAddr = globalAddr.getNewAddress(mem.getLong(globalAddr.add(globalCount * ptrSize * 2 + ptrSize)));
            } else {
                nameAddr = globalAddr.getNewAddress(mem.getInt(globalAddr.add(globalCount * ptrSize * 2)));
                dataAddr = globalAddr.getNewAddress(mem.getInt(globalAddr.add(globalCount * ptrSize * 2 + ptrSize)));
            }

            String name = StringDataInstance.getStringDataInstance(currentProgram.getListing().createData(nameAddr, TerminatedStringDataType.dataType)).getStringValue();

            createLabel(dataAddr, name, true);
        }
        dfGlobalsMap.add(new ArrayDataType(dfGlobalsMapElement, globalCount, ptrSize * 2), "globals", null);

        dtm.addDataType(dfGlobalsMapElement, DataTypeConflictHandler.DEFAULT_HANDLER);
        dtm.addDataType(dfGlobalsMap, DataTypeConflictHandler.DEFAULT_HANDLER);

        currentProgram.getListing().createData(globalAddr, dfGlobalsMap);
    }
}
