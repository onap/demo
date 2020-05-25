.PHONY: all clean helm base cba clean_all
ONBOARDING_PACKAGE_NAME ?= vfw_k8s_demo
CBA_NAME ?= CBA

clean_all:
	$(MAKE) clean
	$(MAKE) all

#Can't use .csar extension or SDC will panic
all: package/ helm base cba
	cd package/ && zip -r $(ONBOARDING_PACKAGE_NAME).zip .
	mv package/$(ONBOARDING_PACKAGE_NAME).zip .

helm: package/
	make -C helm
	mv helm/*.tgz package/

base: package/
	cp base/* package/

cba: package/
	cd cba/ && zip -r $(CBA_NAME).zip .
	mv cba/$(CBA_NAME).zip package/

package/:
	mkdir package/

clean:
	rm -rf package/
	rm -f $(ONBOARDING_PACKAGE_NAME).zip
