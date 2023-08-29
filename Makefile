all:
	-$(MAKE) -C drm
	-$(MAKE) -C videotunnel
	-$(MAKE) -C westeros
	-$(MAKE) -C weston
install:
	-$(MAKE) -C drm install
	-$(MAKE) -C videotunnel install
	-$(MAKE) -C westeros install
	-$(MAKE) -C weston install
clean:
	-$(MAKE) -C drm  clean
	-$(MAKE) -C videotunnel clean
	-$(MAKE) -C westeros clean
	-$(MAKE) -C weston clean
