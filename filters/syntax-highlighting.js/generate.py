#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only

import fnmatch
import io
import json
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import tempfile
from urllib.request import urlopen

import pygments
from pygments.lexers import _iter_lexerclasses

OUTPUT = Path(__file__).resolve().parent

TRANSLATIONS = {
    "actionscript3": ["actionscript"],
    "bdd": ["gherkin"],
    "batch": ["dos"],
    "capnp": ["capnproto"],
    "clojurescript": ["clojure"],
    "cuda": ["cpp"],
    "console": ["shell"],
    "desktop": ["freedesktop"],
    "erl": ["erlang-repl"],
    "fortranfixed": ["fortran"],
    "gas": ["x86asm", "armasm", "avrasm"],
    "html+django": ["django"],
    "html+handlebars": ["handlebars"],
    "html+php": ["php-template"],
    "html+twig": ["twig"],
    "html": ["xml"],
    "json5": ["json"],
    "jsp": ["java"],
    "jsx": ["javascript"],
    "k": ["q"],
    "mips": ["mipsasm"],
    "nasm": ["x86asm"],
    "objective-c++": ["objectivec"],
    "octave": ["matlab"],
    "rhtml": ["erb"],
    "roboconf-graph": ["roboconf"],
    "roboconf-instances": ["roboconf"],
    "slurm": ["bash"],
    "systemd": ["freedesktop"],
    "systemverilog": ["verilog"],
    "tasm": ["x86asm"],
    "text": ["plaintext"],
    "toml": ["ini"],
    "tsx": ["typescript"],
    "wast": ["wasm"],
    "xpp": ["axapta"],
    "xslt": ["xml"],
    "zone": ["dns"],
}

INTERPRETERS = {
    "bash": r"^(?:ba|da|k|z)?sh$",
    "python": r"^(?:python|pypy)[\d.]*$",
    "javascript": r"^node(?:js)?$",
    "perl": r"^perl$",
    "ruby": r"^ruby$",
    "lua": r"^lua[\d.]*$",
    "php": r"^php$",
}

EXTRA_FILENAMES = {
    "asciidoc": ["*.adoc", "*.asciidoc"],
    "bash": ["*.sh.in"],
    "cmake": ["*.cmake.in"],
    "coffeescript": ["*.cson", "*.iced"],
    "django": ["*.jinja"],
    "erb": ["*.erb"],
    "fortran": ["*.f95"],
    "graphql": ["*.gql"],
    "json": ["*.jsonc"],
    "makefile": ["Kbuild", "BSDmakefile"],
    "markdown": ["*.mkd", "*.mkdown"],
    "typescript": ["*.cts", "*.mts"],
}

INSPECT = r"""
import fs from 'node:fs';
import { pathToFileURL } from 'node:url';
const base = process.argv[1];
const { default: hljs } = await import(pathToFileURL(base + '/core.min.js'));
for (const file of fs.readdirSync(base + '/languages').sort()) {
    const { default: grammar } = await import(pathToFileURL(base + '/languages/' + file));
    hljs.registerLanguage(file.replace('.min.js', ''), grammar);
}
const names = new Map(hljs.listLanguages().map(name => [hljs.getLanguage(name), name]));
const info = {};
for (const [language, name] of names) {
    const seen = new Set(), dependencies = new Set();
    function walk(value) {
        if (!value || typeof value !== 'object' || seen.has(value)) return;
        seen.add(value);
        for (const alias of [value.subLanguage || []].flat()) {
            const grammar = hljs.getLanguage(alias);
            if (grammar && grammar !== language) {
                dependencies.add(names.get(grammar));
                walk(grammar);
            }
        }
        Object.values(value).forEach(walk);
    }
    walk(language);
    info[name] = [...dependencies].sort();
}
console.log(JSON.stringify(info));
"""


def pattern_regex(pattern):
    regex = fnmatch.translate(pattern)
    assert regex.startswith("(?s:") and regex.endswith((")\\Z", ")\\z"))
    return regex[4:-3].replace("(?>", "(?:").replace("/", "\\/")


def generate():
    for name in ("core.min.js", "filenames.js"):
        (OUTPUT / name).unlink(missing_ok=True)
    if (OUTPUT / "languages").exists():
        shutil.rmtree(OUTPUT / "languages")

    highlightjs = json.load(urlopen("https://registry.npmjs.org/@highlightjs/cdn-assets/latest"))
    archive = urlopen(highlightjs["dist"]["tarball"]).read()

    with tempfile.TemporaryDirectory() as temporary:
        source = Path(temporary)
        print(f"Using highlight.js {highlightjs['version']} and Pygments {pygments.__version__}")
        with tarfile.open(fileobj=io.BytesIO(archive), mode="r:gz") as tar:
            for member in tar.getmembers():
                if re.fullmatch(r"package/es/(core\.min\.js|languages/[\w-]+\.min\.js)", member.name):
                    target = source / member.name.removeprefix("package/es/")
                    target.parent.mkdir(parents=True, exist_ok=True)
                    target.write_bytes(tar.extractfile(member).read())
        (source / "package.json").write_text('{"type":"module"}\n')
        info = json.loads(subprocess.check_output(
            ["node", "--input-type=module", "-e", INSPECT, str(source)], text=True))
        groups = {}
        matched = 0

        def add(names, patterns, priority=0, primary=True):
            for name in names:
                if name not in info:
                    raise ValueError(f"No highlight.js grammar: {name}")
                groups.setdefault((not primary, -priority, name), set()).update(patterns)

        for lexer in _iter_lexerclasses(plugins=False):
            names = next((TRANSLATIONS[a] for a in lexer.aliases if a in TRANSLATIONS), None)
            if names is None:
                name = next((a for a in lexer.aliases if a in info), None)
                names = [name] if name else []
            if names:
                matched += 1
                add(names, lexer.filenames, lexer.priority)
                add(names, lexer.alias_filenames, lexer.priority, primary=False)

        add(["cpp"], ["*.h"], 0.1)
        for name, patterns in EXTRA_FILENAMES.items():
            add([name], patterns)

        dependencies = {name: deps for name, deps in info.items() if deps}
        output = ["export const rules = ["]
        for (_, _, name), patterns in sorted(groups.items()):
            if patterns:
                regex = "|".join(pattern_regex(p) for p in sorted(patterns))
                output.append(f"[{json.dumps(name)},/^(?:{regex})(?!.)/s],")
        output.extend(["];", "export const interpreters = ["])
        output.extend(f"[{json.dumps(name)},/{regex}/]," for name, regex in INTERPRETERS.items())
        output.extend(["];", "export const dependencies = " + json.dumps(dependencies, separators=(",", ":")) + ";", ""])

        (OUTPUT / "languages").mkdir()
        for asset in [source / "core.min.js", *sorted((source / "languages").glob("*.min.js"))]:
            (OUTPUT / asset.relative_to(source)).write_bytes(asset.read_bytes())
        (OUTPUT / "filenames.js").write_text("\n".join(output))
        print(f"Mapped {matched} Pygments lexers; vendored {len(info)} grammars")
        print(f"Filename rules and dependency data: {(OUTPUT / 'filenames.js').stat().st_size} bytes")


if __name__ == "__main__":
    generate()
