# Minuan

Minuan is a C++ browser for SimVerse. It uses Sunay as its embedded page
renderer and Qt WebEngine (Chromium) for standards-based web rendering. Minuan
opens on a built-in SimVerse gateway, with a project overview, a local HTML6
creation shortcut and links to verified SimVerse project resources.

## Architecture

- Minuan owns the browser window, tabs, address bar, configured HTTPS
  host allowlist, local HTML6 opening and document conversion.
- HTML6Engine reads the document and builds a lightweight element tree.
- DOMProcessor converts supported HTML6 shorthand markup into HTML.
- Sunay embeds Qt WebEngine in each Minuan tab and exposes document loading,
  progress, completion and stop operations.
- Chromium performs HTML/CSS parsing, layout, JavaScript execution, painting
  and compositing.

Sunay is a library dependency of Minuan, not a separate browser.

## Current HTML6 compatibility

The initial conversion pass handles these aliases:

- hd to head, bdy to body
- bttn to button, dv to div, nv to nav
- frm to form, lbl to label, txt to input
- mt to meta, styl to style or external stylesheet link
- scrpt to script

This is a starter compatibility layer, not a complete HTML6 or CSS Next
implementation. A JS-squared interpreter and HTML6 server runtime are not
implemented yet.

## SimVerse gateway

Every new tab opens the built-in SimVerse home page. It links to the published
SimVerse petition, the Sunay project, and the Minuan repository. It clearly
marks the App Store as in development until its catalogue is ready. The home page
can be opened again with the Home button or by entering `simverse://home`.
Choose **Open a local project** on the home page to open an HTML6 or HTML file.

## Allowed websites

Edit browser/websites.json to define HTTPS sites Minuan may navigate to.
The first valid entry is the configured search provider. Google UK and
Google.com are included by default. Plain words entered in the address bar are
searched with Google. Top-level and frame navigation to hosts outside this
list is blocked; subdomains of a configured host are accepted. Plain HTTP
navigation is blocked. Add a domain to this file before Minuan can open it.

## Requirements

- CMake 3.20 or newer
- C++20 compiler
- Qt 6 or Qt 5 with Core, Widgets and WebEngineWidgets
- Git, to fetch the pinned Sunay dependency

## Build

    cmake -S . -B out/build
    cmake --build out/build --config Release

On Windows, build/build.bat configures Visual Studio 2022 and builds the
Release executable. The script runs windeployqt when it is available so the
Qt WebEngine runtime files can be copied next to the executable.

## Open a local document

Launch Minuan and choose Open HTML6. Minuan reads the selected file,
converts supported shorthand markup to HTML, and sends the result to Sunay
with the document's folder as its resource base URL.
