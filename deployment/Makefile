
check_defined = \
	$(strip $(foreach 1,$1, \
		$(call __check_defined,$1)))
__check_defined = \
	$(if $(value $1),, \
		$(error Undefined $1))

all:
	python import_csv.py
	echo "#define SECRET 1234" > global_secrets.h

clean:
	rm -f cc.csv
	rm -f global_secrets.h
