void build(Solution &s)
{
    auto &leptonica = s.addTarget<LibraryTarget>("danbloomberg.leptonica", "1.76.0");
    leptonica += Git("https://github.com/DanBloomberg/leptonica", "{v}");

    leptonica.setChecks("leptonica");

    leptonica +=
        "src/.*\\.c"_rr,
        "src/.*\\.h"_rr,
        "src/endianness.h.in";

    leptonica.Public +=
        "src"_id;

    leptonica.Private += "HAVE_LIBGIF"_d;
    leptonica.Private += "HAVE_LIBJP2K"_d;
    leptonica.Private += "HAVE_LIBJPEG"_d;
    leptonica.Private += "HAVE_LIBPNG"_d;
    leptonica.Private += "HAVE_LIBTIFF"_d;
    leptonica.Private += "HAVE_LIBWEBP"_d;
    leptonica.Private += "HAVE_LIBZ"_d;
    leptonica.Private += "LIBJP2K_HEADER=\"openjpeg.h\""_d;
    leptonica.Public += "HAVE_CONFIG_H"_d;
    leptonica.Private += sw::Shared, "LIBLEPT_EXPORTS"_d;

    leptonica += "org.sw.demo.gif-5"_dep;
    leptonica += "org.sw.demo.jpeg-9"_dep;
    leptonica += "org.sw.demo.uclouvain.openjpeg.openjp2-2"_dep;
    leptonica += "org.sw.demo.glennrp.png-1"_dep;
    leptonica += "org.sw.demo.tiff-4"_dep;
    leptonica += "org.sw.demo.webmproject.webp-*"_dep;

    if (leptonica.Variables["WORDS_BIGENDIAN"] == "1")
        leptonica.Variables["ENDIANNESS"] = "L_BIG_ENDIAN";
    else
        leptonica.Variables["ENDIANNESS"] = "L_LITTLE_ENDIAN";

    leptonica.Variables["APPLE_UNIVERSAL_BUILD"] = "defined (__APPLE_CC__)";

    leptonica.configureFile("src/endianness.h.in", "endianness.h");
    leptonica.writeFileOnce("config_auto.h");

    if (s.Settings.Native.CompilerType == CompilerType::MSVC)
    {
        for (auto *f : leptonica.gatherSourceFiles())
        {
            //f->BuildAs = NativeSourceFile::CPP;
            f->args.push_back("-TP");
        }
    }

    if (s.Settings.TargetOS.Type == OSType::Windows)
        leptonica += "User32.lib"_l, "Gdi32.lib"_l;
}

void check(Checker &c)
{
    auto &s = c.addSet("leptonica");
    s.checkFunctionExists("fmemopen");
    s.checkFunctionExists("fstatat");
    s.checkIncludeExists("dlfcn.h");
    s.checkIncludeExists("inttypes.h");
    s.checkIncludeExists("memory.h");
    s.checkIncludeExists("stdint.h");
    s.checkIncludeExists("stdlib.h");
    s.checkIncludeExists("strings.h");
    s.checkIncludeExists("string.h");
    s.checkIncludeExists("sys/stat.h");
    s.checkIncludeExists("sys/types.h");
    s.checkIncludeExists("unistd.h");
    s.checkTypeSize("size_t");
    s.checkTypeSize("void *");
}
