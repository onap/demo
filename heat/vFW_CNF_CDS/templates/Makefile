<<<<<<< HEAD   (98e3a7 [vFW_CNF_CDS] Add Healthcheck automation)
.PHONY: all clean helm base cba clean_all
=======
.PHONY: all clean helm base clean_all package cba jsonlint_cba
>>>>>>> CHANGE (21757a [vFW_CNF_CDS] Sort generated json files)
DUMMY_ONBOARDING_PACKAGE_NAME ?= vfw_k8s_demo
NATIVE_ONBOARDING_PACKAGE_NAME ?= native_vfw_k8s_demo
CBA_NAME ?= CBA
_type_files = $(wildcard cba/Definitions/*_types.json)


clean_all:
	$(MAKE) clean
	$(MAKE) all

#Can't use .csar extension or SDC will panic
all: package/ helm base cba
	cd package_dummy/ && zip -r $(DUMMY_ONBOARDING_PACKAGE_NAME).zip .
	cd package_native/ && zip -r $(NATIVE_ONBOARDING_PACKAGE_NAME).zip .
	mv package_dummy/$(DUMMY_ONBOARDING_PACKAGE_NAME).zip .
	mv package_native/$(NATIVE_ONBOARDING_PACKAGE_NAME).zip .
	cp $(DUMMY_ONBOARDING_PACKAGE_NAME).zip ../automation/vsp/
	cp $(NATIVE_ONBOARDING_PACKAGE_NAME).zip  ../automation/vsp/

<<<<<<< HEAD   (98e3a7 [vFW_CNF_CDS] Add Healthcheck automation)
helm: package/
	make -C helm
=======
helm: package
	$(MAKE) -C helm
>>>>>>> CHANGE (21757a [vFW_CNF_CDS] Sort generated json files)
	mv helm/helm_*.tgz package_native/
	mv helm/*.tgz package_dummy/

base: package/
	cp base_dummy/* package_dummy/
	cp base_native/* package_native/

cba: package/
	cp -r cba cba_dummy
	sed -i 's/"helm_/"/g' cba_dummy/Definitions/vFW_CNF_CDS.json
	cd cba_dummy/ && zip -r $(CBA_NAME).zip . -x pom.xml .idea/\* target/\*
	cd cba/ && zip -r $(CBA_NAME).zip . -x pom.xml .idea/\* target/\*
	mv cba/$(CBA_NAME).zip package_native/
	mv cba_dummy/$(CBA_NAME).zip package_dummy/

package/:
	mkdir package_dummy/
	mkdir package_native/

clean:
	rm -rf package_dummy/
	rm -rf package_native/
	rm -rf cba_dummy
	rm -f $(DUMMY_ONBOARDING_PACKAGE_NAME).zip
	rm -f $(NATIVE_ONBOARDING_PACKAGE_NAME).zip

jsonlint_cba: $(_type_files)
	@:

#This shouldn't be PHONY, but it's easier now to define this way
.PHONY: $(_type_files)
$(_type_files):
	@which jq expand unexpand sponge >/dev/null
	jq -S . <$@ | unexpand -t 2 | expand -t 4 | sponge $@
