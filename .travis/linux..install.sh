PACKAGES=""

if [ $QT5 ]
then
	PACKAGES="$PACKAGES qtbase5-dev"
else
	PACKAGES="$PACKAGES libqt4-dev"
fi

sudo apt-get install -y $PACKAGES
