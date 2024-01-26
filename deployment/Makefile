
# function to check required arguments
check_defined = \
	$(strip $(foreach 1,$1, \
		$(call __check_defined,$1)))
__check_defined = \
	$(if $(value $1),, \
		$(error Undefined $1))

all:
	cc -fPIC -shared -o encryptlib.so encrypt.c
	cp encryptlib.so ../application_processor/encryptlib.so
	cp encryptlib.so ../component/encryptlib.so
	rm -f ./encryptlib.so
	echo "#define SECRET 1234" > global_secrets.h

clean:
	rm -f global_secrets.h
