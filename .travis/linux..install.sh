PACKAGES="libasound2-dev libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev"

sudo apt-get install -y $PACKAGES

sudo apt-get build-dep -y qt5-default

# Get Qt5
git clone git://code.qt.io/qt/qt5.git
LASTDIR=$(pwd)


  cd qt5
  git checkout 5.4
  perl init-repository --no-webkit --module-subset=qtbase,qtwebsockets,qtmultimedia,qtwidgets,qtsvg
  ./configure -developer-build -opensource -nomake examples -nomake tests -confirm-license
  make -j2

  sudo make install

cd $LASTDIR
