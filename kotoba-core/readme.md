# Compilación

cmake [-DKOTOBA_BUILD_SHARED] ..
cmake --build .

-DKOTOBA_BUILD_SHARED=ON → genera DLL (default)

-DKOTOBA_BUILD_SHARED=OFF → no genera DLL, solo código fuente usable


# GET JMDICT 
	curl -O ftp://ftp.edrdg.org/pub/Nihongo/JMdict.gz
	gunzip JMdict.gz