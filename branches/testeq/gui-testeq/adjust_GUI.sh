#!/bin/bash

#  adjust  the wxformbuilder output cpp file to 
# use xpm icons for the tuning buttons. 

echo "#include \"SoDaLogo_Big.xpm\"" >> temporary_junk.cpp

cat temporary_junk.cpp ../../gui/SoDaBench_GUI.cpp > temporary_junk2.cpp

  sed -e 's/wxT("SoDaLogo_Big.png"), wxBITMAP_TYPE_ANY/SoDaLogo_Big_xpm/g' < temporary_junk2.cpp > SoDaBench_GUI.cxx

rm temporary_junk.cpp temporary_junk2.cpp
