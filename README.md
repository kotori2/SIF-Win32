
SIF-Win32

SIF-Win32 is a Windows port of Love Live! School Idol Festival . It uses open-source older Playground engine which is available in here to make it work. Major modification is also done so that it compatible with latest version of SIF EN (v3.1.3). SIF-Win32 targets Windows XP by default, but it should be compatible with Windows 8.1, even Windows 10 and will uses touchscreen capabilities if available in Windows 7 or above. This project is not in maintenance anymore. Please switch to Playground-SIF .

Is this emulator?

No. It's a native windows application with size around 5MB and no emulation is used here. Because no emulation is used and it's native Windows applicaion, SIF can run in full speed compared when using any Android emulator.

Although the executable size is 5MB, the SIF install data is 50MB and the external data is around 1GB.

Known problems
• 
The engine is very sensitive to race conditions. A temporary workaround for this is to set the program to run in single-core mode.

• 
When extracting many empty files, it takes very long time. It might be caused by NTFS design.

• 
Audio volume slider in Settings doesn't work.

Touchscreen?

Yes, SIF-Win32 have touchscreen support. Touchscreen support is available on devices running at least Windows 7 and capable to do multitouch.

Keyboard support?

Yes, SIF-Win32 also have keyboard support. The keys is configureable.

Compiling and installation
1. 
Grab the latest SIF EN APK and exract file named AppAssets.zip in it. v3.1.3 APK is recommended.

2. 
Clone this project

3. 
Compile this project with Visual Studio 2010 Express or above.

4. 
Navigate to Engine/Porting/Win32/Output/<Debug|Release> and copy all DLLs and EXE in that folder to somewhere else. You may also want to copy libmp3lame.dll and libeay32.dll located somewhere in the project if it's not exist in that folder. Also copy SIF-Win32.json in Engine/Porting/Win32 folder.

5. 
In the new location of the exe, create new directory named install 

6. 
Extacts the contents of AppAssets.zip to <new executable location>/install folder

7. 
Start SampleProject.exe 

GameEngineActivity.xml equivalent?

It's named GE_Keychain.key in SIF-Win32.

Command-line arguments
• 
 -no release sets the game to debug mode. Using debug flag with debug build gives many debug output in the console. Debug mode skips the "Bushiroad" splash screen and shows the detailed output when downloading data.

• 
 -no multicore sets the game to run in single core mode. Might fix some crashes when used.

• 
 -maximize 1 sets the game to run in (almost) fullscreen.

• 
 -xmc <code> to set your own code.

Account Transfer
• 
When transfering account created in SIF-Win32 to iOS (and possibility vice versa), you don't need to clear loveca.

• 
When transfering account created in SIF-Win32 to Android, the loveca must be cleared.

