vf-modules = base firewall packetgen sink
.PHONY: build clean $(vf-modules:=-build) $(vf-modules:-clean)

build: $(vf-modules:=-build)
clean: $(vf-modules:=-clean)

$(vf-modules:=-build): %-build: %-clean
	helm package $(@:-build=)

$(vf-modules:=-clean):
	rm -f $(@:-clean=)-*.tgz
