#/bin/sh
if [ $(uname) == "Darwin" ]; then
    echo "OS type: macOS"
    SODIR=/usr/local/lib
	SOEXT=dylib
	HDIR=/usr/local/include
	SOOWN=$(stat -f "%Su:%Sg" $SODIR)
	SOFLG=$(stat -f "%Mp%Lp" $SODIR)
	HOWN=$(stat -f "%Su:%Sg" $HDIR)
	HFLG=$(stat -f "%Mp%Lp" $HDIR)
else
    echo "OS type: Linux"
	SODIR=/usr/lib
	SOEXT=so
	HDIR=/usr/include
	SOOWN=$(stat -c "%U:%G" $SODIR)
	SOFLG=$(stat -c "%a" $SODIR)
	HOWN=$(stat -c "%U:%G" $HDIR)
	HFLG=$(stat -c "%a" $HDIR)
fi
echo "Copying header files to $HDIR"
sudo cp src/libdes_*.hpp $HDIR
sudo chown $HOWN $HDIR/libdes_*
sudo chmod $HFLG $HDIR/libdes_*.hpp
sudo cp src/incbeta.hpp $HDIR
sudo chown $HOWN $HDIR/incbeta.hpp
sudo chmod $HFLG $HDIR/incbeta.hpp
echo "Done!"

echo "Copying dynamic libray (libdes.$SOEXT) to $SODIR"
sudo mv libdes.$SOEXT $SODIR
sudo chown $SOOWN $SODIR/libdes.$SOEXT
sudo chmod $SOFLG $SODIR/libdes.$SOEXT
echo "Done!"

if [ $(uname) != "Darwin" ]; then
#     sudo update_dyld_shared_cache
# else
    sudo ldconfig
fi

echo "Installation complete!"