# Promovix — Internet Marketing That Works

A marketing agency landing page for **Promovix**: transparent-pricing, multilingual (EN/ES/PT), AI-assisted marketing services — social media, SEO, content, email, and custom strategy.

**Live site:** hosted on Netlify (custom domain)

## Highlights

- **Custom C++ / WebAssembly growth engine** (`engine.cpp` → `wasm/`) — a deterministic simulation that models how organic traffic, social reach, leads, and revenue compound month over month based on channel mix and budget. Runs entirely client-side, no backend, no random numbers — same inputs always produce the same outputs, like a real financial model. Compiled with Emscripten.
- **Interactive "Growth Simulator"** — lets visitors try the engine live and see projected results for their own business.
- **Motion-driven UI** built with self-hosted GSAP (ScrollTrigger, ScrambleText, DrawSVG, ScrollTo plugins), Three.js, and a WebGL globe (cobe) for the multilingual/global-reach section.
- **Six service breakdowns** (custom marketing plan, social media, SEO, content, AI-assisted marketing, email) each with its own explainer video.
- **Zero build step** — plain static HTML/CSS/JS, deployed as-is. `netlify.toml` handles security headers and correct MIME types for the `.wasm` binary.

## Stack

`HTML` `CSS` `JavaScript` `C++` `WebAssembly (Emscripten)` `GSAP` `Three.js` `cobe`

## Structure

```
index.html             # entire site (single-file, no build step)
engine.cpp              # C++ source for the growth simulation engine
wasm/                   # compiled WebAssembly module + JS glue
vendor/                 # self-hosted GSAP, Three.js, cobe
assets/service-videos/  # background-loop videos for each service panel
netlify.toml            # headers + WASM MIME type config for deployment
```
