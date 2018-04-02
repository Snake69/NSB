
all::
	cd server;\
	make;\

	cd gtkclient;\
	make;\

install::
	cd server;\
	make install;\

	cd gtkclient;\
	make install;\

	cd Stats;\
	make install

clean::
	cd server;\
	make clean;\

	cd gtkclient;\
	make clean;\


