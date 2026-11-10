import json
import re
import subprocess
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
frames = [
    [4] * 180 + [5] * 180,
    [i % 8 for i in range(360)],
]
with tempfile.TemporaryDirectory() as tmp:
    source = Path(tmp) / "frames.json"
    output = Path(tmp) / "frames.h"
    source.write_text(json.dumps({"width": 20, "height": 18, "frames": frames}), encoding="utf-8")
    subprocess.run(["python3", str(root / "tools/encode_fmv.py"), str(source), str(output), "--prefix", "unit"], check=True)
    generated = output.read_text(encoding="utf-8")
    streams = re.findall(r"static const uint8_t unit_\d+\[\] = \{(.*?)\};", generated, re.S)
    assert len(streams) == 2
    for expected, body in zip(frames, streams):
        values = [int(x) for x in re.findall(r"\d+", body)]
        decoded = []
        for count, value in zip(values[::2], values[1::2]):
            decoded.extend([value] * count)
        assert decoded == expected
print("Moonlight Holler FMV encoder tests passed.")
