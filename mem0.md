
## Bug ID: MEX-Convert-Overwrite
**Description:** Converting .mex binary to text format inadvertently overwrote the generated text file with its own .mex binary sidecar due to sharing the same extension, causing the conversion to appear broken.
**Resolution:** Updated cmd_mef.cpp and mef_compiler.cpp to use a .extra extension (e.g. model.mex.extra) for the sidecar when the input file is .mex. This safely decouples the text file from the binary sidecar chunks. Also transitioned the sidecar format from a custom SIDX container to standard native ILFF format in mef_exporter.cpp.

## Bug ID: MEX-Export-OBJ-Error
**Description:** The GUI was incorrectly trying to invoke export-obj when exporting .mex and .mef files to OBJ via the right-click menu, leading to unknown subcommand errors since the CLI expects export. Additionally, .mex was not wired up to correctly append the -o <output> argument.
**Resolution:** Changed the GUI action string from export-obj to export, and ensured that both mex export and mex dump correctly format the command line arguments passing -o to the CLI tool. Also added dynamic parsing of the 4-byte header ILFF in gui_main.cpp to correctly determine if a .mef/.mex file should open in 3D/Hex view (binary) or Text view (ascii) instead of blindly relying on the extension.

## Feature Request: GUI Rename and Build Rigid tweaks
**Description:** Renamed menu actions and adjusted uild-rigid behavior as requested.
**Resolution:** Renamed Build Rigid Model to Build Model. Renamed Export to OBJ to Export to Obj. Renamed Export to Text to Export to Mef(Text). Grouped Info and Dump under an Info & Dump submenu. Updated cmd_mef.cpp to output to the same filename (overwriting) instead of appending _rigid.mef. Updated gui_main.cpp to show the custom loading bar text Completing Build model finding attachments textures.... Please wait when executing uild-rigid.

## Feature Request: GUI Rename Info & Dump
**Description:** Renamed the Info & Dump submenu to Details as per user preference.
**Resolution:** Renamed in gui_main.cpp and compiled the Release build.
