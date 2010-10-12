@rem @echo off

@rem  Dick with settings
call ..\scripts\automated_build_scripts\update_settings.bat

@rem  Set your build type here.  Release = no debugging symbols.  RelWithDebInfo = Optimized with debugging symbols. Debug = FAT AND SLOW, MUCH LIKE YOUR MOTHER
@set BUILDTYPE=Release

@rem TODO: Turn this into a horrific python monstrosity.
@rem 	Gather hooks in order to generate documentation.
@rem 	Will fail without cygwin, so find a better way.
@rem find newview/ -name "*.cpp" -print | C:\lua5.1\lua ../scripts/GetHooks.lua

@rem  Generator: VC80 = Visual C++ 2005, VC90 = Visual C++ 2008 (use this)
@set GEN="VC80"
python ./develop.py --type=%BUILDTYPE% -G "%GEN%" configure -DPACKAGE:BOOL=TRUE -DLL_TESTS:BOOL=FALSE
@rem python ./develop.py --type=%BUILDTYPE% -G "%GEN%" build
rem VERBOSE=1
@pause