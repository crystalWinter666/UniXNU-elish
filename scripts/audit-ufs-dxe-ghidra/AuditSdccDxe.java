import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import ghidra.util.task.TaskMonitor;

import java.util.LinkedHashMap;
import java.util.Map;

public class AuditSdccDxe extends ghidra.app.script.GhidraScript {
    private final Map<String, Function> functions = new LinkedHashMap<>();

    @Override
    public void run() throws Exception {
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);

        String[][] targets = {
            {"0x12468", "SDCC IOMMU LocateProtocol failed"},
            {"0x1248f", "SDC2 IOMMU domain create failed"},
            {"0x124b0", "SDC2 IOMMU domain configure failed"},
            {"0x124df", "SDC2 IOMMU domain attach ARID 0x0 failed"},
            {"0x12509", "SDC4 IOMMU domain create failed"},
            {"0x12529", "SDC4 IOMMU domain configure failed"},
            {"0x1254d", "SDC4 IOMMU domain attach ARID 0x1 failed"}
        };

        for (String[] target : targets) {
            Address address = toAddr(Long.decode(target[0]));
            println("\n=== STRING " + target[0] + " " + target[1] + " ===");
            ReferenceIterator refs = currentProgram.getReferenceManager().getReferencesTo(address);
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
