Boost python build line
..\boostbuild\bin\b2.exe --build-type=complete --with-python --user-config=user-config.jam release msvc stage
use file user-config.jam attached to this project

#define BOOST_PYTHON_STATIC_LIB

./b2 --with-python --user-config=user-config.jam