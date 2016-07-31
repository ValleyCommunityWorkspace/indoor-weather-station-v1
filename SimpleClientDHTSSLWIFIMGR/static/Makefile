# Haven't considered any sort of minifying or compression.

AWK=/usr/local/bin/awk
XXD=/usr/bin/XXD

REPLACEMENT_STRING=THINGYTHINGS

network-selector.h: firsthalf.h secondhalf.h
	cat firsthalf.h secondhalf.h > network-selector.h
	rm -f firsthalf.h secondhalf.h

firsthalf.h: network-selector.html
	echo "constexpr const char network_selector_first[] = {" > $@
	${AWK} '/${REPLACEMENT_STRING}/{exit};1' $< | ${XXD} -i >> $@
	echo "};" >> $@

secondhalf.h: network-selector.html
	echo "constexpr const char network_selector_second[] = {" > $@
	${AWK} '/${REPLACEMENT_STRING}/{x=1;next};x' $< | ${XXD} -i >> $@
	echo "};" >> $@

clean:
	rm -f network-selector.h firsthalf.h secondhalf.h
