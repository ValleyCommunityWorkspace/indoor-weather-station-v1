# Haven't considered any sort of minifying or compression.

XXD=/usr/bin/XXD

network-selector.h: network-selector.html
	echo "constexpr const char network_selector[] = {" > $@
	${XXD} -i < $< >> $@
	echo "  , 0x00};" >> $@

clean:
	rm -f network-selector.h
