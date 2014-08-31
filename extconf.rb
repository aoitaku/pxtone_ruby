require "mkmf"
if have_header('pxtoneWin32.h') and have_library('pxtone')
  create_makefile("Pxtone")
end
