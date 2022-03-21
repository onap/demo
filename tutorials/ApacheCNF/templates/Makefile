.PHONY: all clean helm base clean_all package cba jsonlint_cba
NATIVE_ONBOARDING_PACKAGE_NAME ?= native_cnf_k8s_demo
CBA_NAME ?= CBA
_type_files = $(wildcard cba/Definitions/*_types.json)


clean_all:
	$(MAKE) clean
	$(MAKE) all

#Can't use .csar extension or SDC will panic
all: package helm base cba
	cd package_native/ && zip -r $(NATIVE_ONBOARDING_PACKAGE_NAME).zip .
	mv package_native/$(NATIVE_ONBOARDING_PACKAGE_NAME).zip .
	cp $(NATIVE_ONBOARDING_PACKAGE_NAME).zip  ../automation/vsp/

helm: package
	$(MAKE) -C helm
	mv helm/helm_*.tgz package_native/

base: package
	cp base_native/* package_native/

cba: package
	cd cba/ && zip -r $(CBA_NAME).zip . -x pom.xml .idea/\* target/\*
	mv cba/$(CBA_NAME).zip package_native/

package:
	mkdir package_native/

clean:
	rm -rf package_native/
	rm -f $(NATIVE_ONBOARDING_PACKAGE_NAME).zip

jsonlint_cba: $(_type_files)
	@:

#This shouldn't be PHONY, but it's easier now to define this way
.PHONY: $(_type_files)
$(_type_files):
	@which jq expand unexpand sponge >/dev/null
	jq -S . <$@ | unexpand -t 2 | expand -t 4 | sponge $@
