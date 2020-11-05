vf-modules = base_template vpkg vfw vsn
.PHONY: build clean $(vf-modules:=-build) $(vf-modules:-clean) $(vf-modules:=-package)

package: $(vf-modules:=-package)
build: $(vf-modules:=-build)
clean: $(vf-modules:=-clean)

$(vf-modules:=-package): %-package: %-build
	mv $(@:package=)*.tgz helm_$(@:-package=).tgz
	cp helm_$(@:-package=).tgz $(@:-package=)_cloudtech_k8s_charts.tgz

$(vf-modules:=-build): %-build: %-clean
	helm package $(@:-build=)

$(vf-modules:=-clean):
	rm -f $(@:-clean=)-*.tgz
	rm -f helm_$(@:-clean=).tgz
	rm -f $(@:-clean=)_cloudtech_k8s_charts.tgz
