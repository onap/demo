.PHONY: all clean Helm base CBA clean_all
ONBOARDING_PACKAGE_NAME ?= fiveg
CBA_NAME ?= CBA

clean_all:
	$(MAKE) clean
	$(MAKE) all

#Can't use .csar extension or SDC will panic
all: package/ Helm base CBA
	cd package/ && zip -r $(ONBOARDING_PACKAGE_NAME).zip .
	mv package/$(ONBOARDING_PACKAGE_NAME).zip .

Helm: package/
	make -C Helm
	mv Helm/*.tgz package/

base: package/
	cp base/* package/

CBA: package/
	cd CBA/ && zip -r $(CBA_NAME).zip .
	mv CBA/$(CBA_NAME).zip package/

package/:
	mkdir package/

clean:
	rm -rf package/
	rm -f $(ONBOARDING_PACKAGE_NAME).zip
