// Dump GUID-sized data and code references around the suspected UFS/SDCC GUIDs.
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import ghidra.util.task.TaskMonitor;

import java.util.LinkedHashMap;
import java.util.Map;

public class AuditGuidXrefs extends ghidra.app.script.GhidraScript {
    private final Map<String, Function> functions = new LinkedHashMap<>();

    private String bytesAt(Address address, int length) throws Exception {
        byte[] bytes = new byte[length];
        currentProgram.getMemory().getBytes(address, bytes);
        StringBuilder result = new StringBuilder();
        for (byte value : bytes) {
            result.append(String.format("%02x", value & 0xff));
        }
        return result.toString();
    }

    private void inspect(String value) throws Exception {
        Address address = toAddr(Long.decode(value));
        println("\n=== ADDRESS " + value + " BYTES " + bytesAt(address, 24) + " ===");
        Function directFunction = getFunctionAt(address);
        if (directFunction != null) {
            functions.put(directFunction.getEntryPoint().toString(), directFunction);
            println("function=" + directFunction.getEntryPoint() + " "
                + directFunction.getName());
        }
        ReferenceIterator refs = currentProgram.getReferenceManager().getReferencesTo(address);
        if (!refs.hasNext()) {
            println("NO_XREF");
        }
        while (refs.hasNext()) {
            Reference ref = refs.next();
            Function function = getFunctionContaining(ref.getFromAddress());
            println("xref=" + ref.getFromAddress() + " type=" + ref.getReferenceType()
                + " function=" + (function == null ? "<none>" : function.getEntryPoint()
                + " " + function.getName()));
            if (function != null) {
                functions.put(function.getEntryPoint().toString(), function);
            }
        }

    }

    @Override
    public void run() throws Exception {
        String[] targets = getScriptArgs();
        if (targets.length == 0) {
            targets = new String[] {"0x17268", "0x19248"};
        }
        for (String address : targets) {
            inspect(address);
        }

        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        for (Function function : functions.values()) {
            println("\n=== DECOMPILE " + function.getEntryPoint() + " " + function.getName()
                + " ===");
            DecompileResults result = decompiler.decompileFunction(function, 120, TaskMonitor.DUMMY);
            if (result.decompileCompleted()) {
                println(result.getDecompiledFunction().getC());
            } else {
                println("DECOMPILE_FAILED: " + result.getErrorMessage());
            }
        }
        decompiler.dispose();
    }
}
