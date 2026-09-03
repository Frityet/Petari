#!/usr/bin/env python3
"""Verify restoration provenance without pretending to perform target objdiff."""

from pathlib import Path
import hashlib
import json
import re
import subprocess

NOTES = Path(__file__).resolve().parent
ROOT = NOTES.parents[2]
DATA = json.loads((NOTES / "source-correspondence.json").read_text())


def historical(path):
    data = subprocess.check_output(
        ["git", "show", DATA["historical_revision"] + ":" + path], cwd=ROOT
    )
    assert hashlib.sha256(data).hexdigest() == DATA["historical_sha256"][path]
    return data.decode()


def rename(text, names):
    return re.sub(r"\b(" + "|".join(names) + r")\b", lambda match: names[match.group()], text)


def tokens(text):
    # Preserve quoted literals; ignore only formatting whitespace elsewhere.
    return re.findall(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[^\s]', text)


collector_path = "src/Game/NPC/RunawayRabbitCollect.cpp"
collector = historical(collector_path)
collector = collector[collector.index("void RunawayRabbitCollect::init("):]
collector = collector.replace("mHasAppearedTico[", "_B0[")
collector = collector.replace("->mHasAppearedTico", "->_F5")
collector = rename(collector, DATA["collector_members"])
collector = rename(collector, {key: DATA["rabbit_members"][key] for key in ("mGroupId", "mIsCaughtable")})
current = (ROOT / collector_path).read_text()
current = current[current.index("void RunawayRabbitCollect::init("):]
current = rename(current, DATA["collector_bgm_frames"])
assert tokens(collector) == tokens(current), "collector function bodies differ from the documented mapping"
print("PASS: all collector bodies from init through destructor match the historical source after documented renames/constants")

rabbit_path = "src/Game/NPC/RunawayRabbit.cpp"
rabbit = rename(historical(rabbit_path), DATA["rabbit_members"])
rabbit = rabbit.replace('#include "Game/NPC/TrickRabbit.hpp"', '#include "Game/NPC/TrickRabbitUtil.hpp"')
assert rabbit == (ROOT / rabbit_path).read_text(), "rabbit source contains changes beyond the documented member renames and utility include"
print("PASS: complete rabbit source is byte-identical to the historical source after documented member renames and utility include")

for actor in ("RunawayRabbitCollect", "RunawayRabbit"):
    path = "include/Game/NPC/" + actor + ".hpp"
    original = subprocess.check_output(
        ["git", "show", DATA["upstream_baseline_revision"] + ":" + path], cwd=ROOT, text=True
    )
    expected = original.replace("    virtual void init(", "    virtual ~" + actor + "();\n    virtual void init(", 1)
    assert expected == (ROOT / path).read_text(), "header change exceeds the destructor declaration"
print("PASS: both headers preserve the upstream declarations and layout, adding only destructor declarations")
print("No target compilation or binary matching was performed by this check.")
