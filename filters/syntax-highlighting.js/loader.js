/* SPDX-License-Identifier: GPL-2.0-only */
(function () {
	const base = new URL(".", document.currentScript.src);

	document.addEventListener("DOMContentLoaded", async function () {
		const code = document.querySelector("#cgit table.blob td.lines pre > code");
		if (!code || code.children.length)
			return;
		const style = document.createElement("link");
		style.rel = "stylesheet";
		style.href = new URL("style.css", base).href;
		document.head.appendChild(style);
		const crumb = document.querySelector("#cgit .path a:last-child");
		const filename = crumb?.textContent.split("/").pop() || "";
		const text = code.textContent;
		try {
			const { rules, interpreters, dependencies } = await import(new URL("filenames.js", base).href);
			let names = rules.filter(([, pattern]) => pattern.test(filename)).map(([name]) => name);
			const shebang = !names.length && /^#![ \t]*(\S+)([^\r\n]*)/.exec(text);
			if (shebang) {
				let command = shebang[1];
				if (command.split("/").pop() === "env") {
					const args = shebang[2].trim();
					const split = /^(?:-S[ \t]*|--split-string(?:=|[ \t]+))/.exec(args);
					const word = split
						? /^(?:"([\w./+-]+)"|'([\w./+-]+)'|([\w./+-]+))(?:[ \t]|$)/.exec(args.slice(split[0].length))
						: /^([\w./+-]+)$/.exec(args);
					command = word?.slice(1).find(Boolean) || "";
				}
				const interpreter = command.split("/").pop();
				names = interpreters.filter(([, pattern]) => pattern.test(interpreter)).map(([name]) => name);
			}
			names = [...new Set(names)].filter(name => name !== "plaintext");
			if (!names.length)
				return;

			const { default: hljs } = await import(new URL("core.min.js", base).href);
			const needed = new Set(names.flatMap(name => [name, ...(dependencies[name] || [])]));
			await Promise.all([...needed].map(async name => {
				const { default: grammar } = await import(new URL(`languages/${name}.min.js`, base).href);
				hljs.registerLanguage(name, grammar);
			}));
			let best;
			for (const language of names) {
				const result = hljs.highlight(text, { language, ignoreIllegals: names.length === 1 });
				if (!result.illegal && (!best || result.relevance > best.relevance))
					best = result;
			}
			if (best) {
				code.innerHTML = best.value;
				code.classList.add("hljs");
			}
		} catch (error) {
			console.warn("Syntax highlighting failed:", error);
		}
	});
})();
