function args_contains(element)
  for _, value in pairs(_ARGS) do
    if value == element then
      return true
    end
  end
  return false
end

newoption {
    trigger = "prefix",
    value = "/usr",
    description = "Installation prefix"
}

if not _OPTIONS["prefix"] then
    _OPTIONS["prefix"] = "/usr"
end

newaction {
    trigger = "install",
    description = "Install binary files to system location(s) [you have to build the project first]",
    execute = function()
        if not os.is("windows") then
            prefix = _OPTIONS["prefix"]
            print("Installing newsoul binary")
            binpath = path.join(prefix, "bin")
            os.mkdir(binpath)
            os.copyfile("bin/newsoul", binpath)
            print("Installing newsoul man page")
            manpath = path.join(prefix, "share/man/man1")
            os.mkdir(manpath)
            os.copyfile("../src/newsoul.1", manpath)
            print("Installing config template")
            tmplpath = path.join(prefix, "share/newsoul")
            os.mkdir(tmplpath)
            os.copyfile("../src/config.xml.tmpl", tmplpath)
            print("Installing systemd service file")
            syspath = path.join(prefix, "lib/systemd/system")
            os.mkdir(syspath)
            os.copyfile("../scripts/newsoul@.service", syspath)
        end
    end
}

function include()
    if os.is("bsd") then
        includedirs {"/usr/local/include/libxml2", "/usr/local/include/db5"}
    else
        includedirs {"/usr/include/libxml2"}
    end
end

function link()
    include()
    links {
        "tag", "z", "event", "nettle", "db_cxx", "xml2",
        "newsoul-lib", "efsw", "NewNet", "utils", "json-c"
    }
    if os.is("bsd") then
        libdirs {"/usr/local/lib/db5"}
        links {"iconv"}
    end

    if args_contains("kqueue") then
        links {"kqueue"}
    end
    if not os.is("windows") and not os.is("haiku") then
        links {"pthread"}
    end
    if os.is("macosx") then
        links {"CoreFoundation.framework", "CoreServices.framework"}
    end
end

solution "newsoul"
    targetdir "bin"
    configurations {"release", "debug"}
    language "C++"
    flags {"ExtraWarnings"}
    buildoptions {"-std=c++11"}

    configuration "debug"
        defines {"DEBUG"}
        flags {"Symbols"}
        buildoptions {"-ggdb3", "-O0", "-fno-inline"}

    configuration "release"
        defines {"NDEBUG"}
        flags {"Optimize"}

    project "newsoul"
        kind "ConsoleApp"
        files {"../src/main.cpp"}
        link()

    project "newsoul-tests"
        kind "ConsoleApp"
        files {"../tests/**.cpp"}
        link()

    project "newsoul-lib"
        kind "StaticLib"
        targetdir "lib"
        files {"../src/*.cpp"}
        excludes {"../src/main.cpp"}
        include()

    project "utils"
        kind "StaticLib"
        targetdir "lib"
        files {"../src/utils/*.cpp"}

    project "NewNet"
        kind "StaticLib"
        targetdir "lib"
        files {"../src/NewNet/*.cpp"}
        defines {"NN_PTR_DEBUG", "NN_PTR_DEBUG_ASSERT"}

    project "efsw"
        if os.is("windows") then
            osfiles = "../src/efsw/src/efsw/platform/win/*.cpp"
        else
            osfiles = "../src/efsw/src/efsw/platform/posix/*.cpp"
        end

        -- This is for testing in other platforms that don't support kqueue
        if args_contains("kqueue") then
            defines {"EFSW_KQUEUE"}
            printf("Forced Kqueue backend build.")
        end

        -- Activates verbose mode
        if args_contains("verbose") then
            defines {"EFSW_VERBOSE"}
        end

        if os.is("macosx") then
            -- Premake 4.4 needed for this
            if not string.match(_PREMAKE_VERSION, "^4.[123]") then
                local ver = os.getversion();

                if not (ver.majorversion >= 10 and ver.minorversion >= 5) then
                    defines {"EFSW_FSEVENTS_NOT_SUPPORTED"}
                end
            end
        end

		kind "StaticLib"
		targetdir "lib"
		includedirs {"../src/efsw/include", "../src/efsw/src"}
		files {"../src/efsw/src/efsw/*.cpp", osfiles}

		configuration "debug"
			buildoptions {"-pedantic -Wno-long-long"}
