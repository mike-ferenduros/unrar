#
#  Be sure to run `pod spec lint unrar.podspec' to ensure this is a
#  valid spec and to remove all comments including this before submitting the spec.
#
#  To learn more about Podspec attributes see http://docs.cocoapods.org/specification.html
#  To see working Podspecs in the CocoaPods repo see https://github.com/CocoaPods/Specs/
#

Pod::Spec.new do |s|

  s.name         = "unrar"
  s.version      = "5.0.0"
  s.summary      = "Unrar library"
  s.description  = "Library for reading RAR archives"
  s.homepage     = "https://www.rarlab.com"

  s.license      = { :type => "Freeware", :file => "license.txt" }
  s.author    = "Alexander L. Roshal"

  s.source       = { :git => "https://github.com/mike-ferenduros/unrar.git" }
  s.source_files  = "*.hpp", "rar.cpp", "dll.cpp",
                      "strlist.cpp",
                      "strfn.cpp",
                      "pathfn.cpp",
                      "smallfn.cpp",
                      "global.cpp",
                      "file.cpp",
                      "filefn.cpp",
                      "filcreat.cpp",
                      "archive.cpp",
                      "arcread.cpp",
                      "unicode.cpp",
                      "system.cpp",
                      "isnt.cpp",
                      "crypt.cpp",
                      "crc.cpp",
                      "rawread.cpp",
                      "encname.cpp",
                      "resource.cpp",
                      "match.cpp",
                      "timefn.cpp",
                      "rdwrfn.cpp",
                      "consio.cpp",
                      "options.cpp",
                      "errhnd.cpp",
                      "rarvm.cpp",
                      "secpassword.cpp",
                      "rijndael.cpp",
                      "getbits.cpp",
                      "sha1.cpp",
                      "sha256.cpp",
                      "blake2s.cpp",
                      "hash.cpp",
                      "extinfo.cpp",
                      "extract.cpp",
                      "volume.cpp",
                      "list.cpp",
                      "find.cpp",
                      "unpack.cpp",
                      "headers.cpp",
                      "threadpool.cpp",
                      "rs16.cpp",
                      "cmddata.cpp",
                      "ui.cpp",
                      "filestr.cpp",
                      "recvol.cpp",
                      "rs.cpp",
                      "scantree.cpp",
                      "qopen.cpp"

  s.preserve_paths = "*.cpp"

  s.public_header_files = "dll.hpp"

  s.pod_target_xcconfig = { "OTHER_CPLUSPLUSFLAGS" => "$(inherited) -Wno-logical-op-parentheses -Wno-conversion  -Wno-parentheses -Wno-unused-function -Wno-unused-variable -Wno-conditional-uninitialized -Wno-switch -DRARDLL" }

end
