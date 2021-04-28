.PHONY: all clean helm base cba clean_all package
DUMMY_ONBOARDING_PACKAGE_NAME ?= vfw_k8s_demo
NATIVE_ONBOARDING_PACKAGE_NAME ?= native_vfw_k8s_demo
CBA_NAME ?= CBA

clean_all:
	$(MAKE) clean
	$(MAKE) all

#Can't use .csar extension or SDC will panic
all: package helm base cba
	cd package_dummy/ && zip -r $(DUMMY_ONBOARDING_PACKAGE_NAME).zip .
	cd package_native/ && zip -r $(NATIVE_ONBOARDING_PACKAGE_NAME).zip .
	mv package_dummy/$(DUMMY_ONBOARDING_PACKAGE_NAME).zip .
	mv package_native/$(NATIVE_ONBOARDING_PACKAGE_NAME).zip .
	cp $(DUMMY_ONBOARDING_PACKAGE_NAME).zip ../automation/vsp/
	cp $(NATIVE_ONBOARDING_PACKAGE_NAME).zip  ../automation/vsp/

helm: package
	make -C helm
	mv helm/helm_*.tgz package_native/
	mv helm/*.tgz package_dummy/

base: package
	cp base_dummy/* package_dummy/
	cp base_native/* package_native/

cba: package
	cp -r cba cba_dummy
	sed -i 's/"helm_/"/g' cba_dummy/Definitions/vFW_CNF_CDS.json
	cd cba_dummy/ && zip -r $(CBA_NAME).zip . -x pom.xml .idea/\* target/\*
	cd cba/ && zip -r $(CBA_NAME).zip . -x pom.xml .idea/\* target/\*
	mv cba/$(CBA_NAME).zip package_native/
	mv cba_dummy/$(CBA_NAME).zip package_dummy/

package:
	mkdir package_dummy/
	mkdir package_native/

clean:
	rm -rf package_dummy/
	rm -rf package_native/
	rm -rf cba_dummy
	rm -f $(DUMMY_ONBOARDING_PACKAGE_NAME).zip
	rm -f $(NATIVE_ONBOARDING_PACKAGE_NAME).zip
