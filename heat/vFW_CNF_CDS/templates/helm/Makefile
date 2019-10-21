vf-modules = base firewall packetgen sink
.PHONY: build clean $(vf-modules:=-build) $(vf-modules:-clean) $(vf-modules:=-package)

package: $(vf-modules:=-package)
build: $(vf-modules:=-build)
clean: $(vf-modules:=-clean)
	rm -f *_cloudtech_k8s_charts.tgz

$(vf-modules:=-package): %-package: %-build
	mv $(@:package=)*.tgz $(@:-package=)_cloudtech_k8s_charts.tgz

$(vf-modules:=-build): %-build: %-clean
	helm package $(@:-build=)

$(vf-modules:=-clean):
	rm -f $(@:-clean=)-*.tgz
