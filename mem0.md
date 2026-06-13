
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

## Feature Request: GUI Export Obj tweaks
**Description:** User requested to rename "Export to Obj" to "Export to OBJ+MTL+TGA" and have text MEF/MEX export prompt for a folder.
**Resolution:** Updated gui_main.cpp to rename the export menu items to "Export to OBJ+MTL+TGA". Added `QFileDialog::getExistingDirectory` logic for text mef/mex exports so they ask for a folder, matching the binary export behavior. Built and ran Release version.

## Bug ID: MEX-Lighting-Fix
**Description:** When rotating objects in the 3D viewport, the lighting incorrectly shifted non-uniformly because the fragment shader was using a point light tied to FragPos which varied under object translation. Additionally, backfaces became pitch black due to single-sided lighting.
**Resolution:** Replaced the point light with a fixed directional headlamp in gui_main.cpp's fragment shader, and added abs() to the dot product to implement two-sided lighting so both the front and back of the geometry stay uniformly lit regardless of rotation angle. Built in Release mode.

## Bug ID: MEF-Recursive-ATTA-Scan
**Description:** The GUI editor did not recursively load ATTA (attachment) models and their textures when a base MEF file was opened. The 'build-rigid' CLI tool also lost textures when compiling the rigid model because it stripped the PMTL/TAMC chunks without generating a proper sidecar. This caused users to see missing geometries and textures when examining assembled levels like 435_01_1.mef.
**Resolution:** Implemented loadMefRecursive in gui_main.cpp. The GUI now parses ATTA matrices and naturally handles the hierarchical structure, natively scanning for attachments and their textures directly into the viewport. This makes 'build-rigid' unnecessary for merely viewing full levels with textures.
