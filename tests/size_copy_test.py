"""The size pass must never share an IRQ-reachable, volatile, or out-of-range copy."""
import importlib.util
from pathlib import Path
from tempfile import TemporaryDirectory

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("size_fns", root / "tools/size_fns.py")
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

intrinsic = "call void @llvm.memcpy.p0i8.p0i8.i32(i8* %d, i8* %s, i32 64, i32 1, i1 false)"
plain = "call i8* @memcpy(i8* %d, i8* %s, i32 64)"

def body(name, content):
    return f"define void @{name}(i8* %d, i8* %s) #0 {{\n  {content}\n  ret void\n}}\n"

source = body("fm1_alnk0_irq", "call void @project_capture(i8* %d, i8* %s)")
source += body("project_capture", intrinsic + "\n  " + plain)
source += body("song_saved_index", "\n  ".join((intrinsic, plain,
    intrinsic.replace("false", "true"), intrinsic.replace("i32 64", "i32 4"),
    intrinsic.replace("i32 64", "i32 512"), plain.replace("i32 64", "i32 %n"))))
source += body("ui_copy_bytes", intrinsic) + body("ui_copy", plain)
source += 'attributes #0 = { nounwind }\n'
with TemporaryDirectory() as tmp:
    a, b = Path(tmp) / "in.ll", Path(tmp) / "out.ll"
    a.write_text(source)
    assert mod.main(a, b) == 0
    result = b.read_text()
    parts = {m[1]: m[0] for m in mod.IR_BODY.finditer(result)}
    for name in ("fm1_alnk0_irq", "project_capture", "ui_copy_bytes", "ui_copy"):
        before = next(m[0].split("{", 1)[1] for m in mod.IR_BODY.finditer(source) if m[1] == name)
        after = next(m[0].split("{", 1)[1] for m in mod.IR_BODY.finditer(result) if m[1] == name)
        assert before == after, name
    changed = parts["song_saved_index"]
    assert "@ui_copy_bytes(" in changed and "@ui_copy(" in changed
    assert intrinsic.replace("false", "true") in changed
    assert intrinsic.replace("i32 64", "i32 4") in changed
    assert intrinsic.replace("i32 64", "i32 512") in changed
    assert plain.replace("i32 64", "i32 %n") in changed
    assert mod.isr_reach(source) == mod.isr_reach(result)
print("PASS bounded main-loop copies shared; IRQ closure, volatile and other sizes unchanged")
