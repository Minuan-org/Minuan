# Minuan

Minuan is a C++ browser for SimVerse. It uses Sunay as its embedded page
renderer and Qt WebEngine (Chromium) for standards-based web rendering. Minuan
opens on a built-in SimVerse gateway, with a project overview, a local HTML6
creation shortcut and links to verified SimVerse project resources.

## Architecture

- Minuan owns the browser window, tabs, address bar, configured HTTPS
  host allowlist, bookmarks, downloads, privacy toggles, local HTML6 opening
  and document conversion.
- HTML6Engine reads the document and builds a lightweight element tree.
- HTML6Validator checks custom names and common standard HTML names before conversion.
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

Minuan follows the HTML6 v1.0 document layout and validates section order,
server placement, cloud/local multiplayer rules, and request-handler paths.
The `mt` title form sets the browser tab title, and the documented
`s_ss`/`ss` button attributes compose the sign-in label.

This is still a compatibility layer, not a complete HTML6 language runtime.
Minuan parses and stores `srv`/`nrq` declarations, but does not host their
responses as a network backend. CSS Next, JS-squared execution, and Chrono
multiplayer are not implemented.

## SimVerse gateway

Every new tab opens the built-in SimVerse home page. It links to the published
SimVerse petition, the Sunay project, and the Minuan repository. It clearly
marks the App Store as in development until its catalogue is ready. The home page
can be opened again with the Home button or by entering `simverse://home`.
Choose **Open a local project** on the home page to open an HTML6 or HTML file.

Minuan also supports opening links in new tabs, saving approved-site bookmarks,
handling downloads with a Save dialog, and switching JavaScript or image loading
off for the current tab. The address bar keeps the SimVerse home label and the
selected local-file name while Sunay renders those documents. Keyboard shortcuts
include Ctrl+T (new tab), Ctrl+W (close tab), Ctrl+L (focus address bar),
Ctrl+D (bookmark), and Ctrl+O (open file).

## Allowed websites

Edit browser/websites.json to define HTTPS sites Minuan may navigate to.
Google UK is used for searches. Google UK, Google.com and GitHub are
included by default. Plain words entered in the address bar are searched with
Google. Top-level and frame navigation to hosts outside this list is blocked;
subdomains of a configured host are accepted. Plain HTTP, URL-embedded
credentials, and non-standard HTTPS ports are blocked. Add a domain to this
file before Minuan can open it.

## Requirements

- CMake 3.20 or newer
- C++20 compiler
- Qt 6 or Qt 5 with Core, Widgets and WebEngineWidgets
- Git, to fetch the pinned Sunay dependency

## Build

    cmake -S . -B out/build
    cmake --build out/build --config Release

On Windows, build/build.bat configures Visual Studio 2022, builds and tests
the Release executable, then runs windeployqt to copy the Qt and Qt WebEngine
runtime, resources and locale files beside the executable. The portable folder
must stay together when moving Minuan to another computer. Windows also needs
the Microsoft Visual C++ x64 Redistributable (version 14.28 or later). Get the
official installer from https://aka.ms/vc14/vc_redist.x64.exe if it is not
already installed.

## Open a local document

Launch Minuan and choose Open HTML6. Minuan reads the selected file,
converts supported shorthand markup to HTML, and sends the result to Sunay
with the document's folder as its resource base URL.
